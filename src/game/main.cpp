

#include <rynx/application/application.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/simulation.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>

#include <rynx/input/mapped_input.hpp>

#include <rynx/std/timer.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>

#include <rynx/graphics/framebuffer.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>

#include <game/sample_application.hpp>
#include <rynx/audio/audio.hpp>

#include <iostream>
#include <thread>

#include <cmath>

#include <rynx/filesystem/virtual_filesystem.hpp>

// for piano
#include <rynx/application/components.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/FileSelector.hpp>
#include <rynx/menu/Frame.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/menu/ListSelector.hpp>
#include <rynx/menu/Slider.hpp>

bool is_white_key(int midiKey) {
  int32_t whites_in_octave = (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) |
                             (1 << 7) | (1 << 8) | (1 << 10);
  int index_in_octave = (midiKey + 3) % 12;
  return (whites_in_octave >> index_in_octave) & 1;
};

std::vector<float> x_offsets(120, 0.0f);

class Recording {
public:
  struct Event {
    int32_t m_timeStartMs = 0;
    int32_t m_timeEndMs = 0;
    int32_t m_midiKey = 0;
    int32_t m_velocity = 0;
    rynx::id m_ecsId;

    bool m_userHit = false;

    bool operator==(const Event &other) const {
      return m_midiKey == other.m_midiKey && m_timeEndMs == other.m_timeEndMs;
    }

    bool by_start(const Event &other) const {
      return m_timeStartMs < other.m_timeStartMs;
    }
    bool by_end(const Event &other) const {
      return m_timeEndMs < other.m_timeEndMs;
    }
  };

  Recording() {}

  std::pair<int32_t, int32_t> find_nearest_note_start(int32_t midiKey) {
    int32_t best = 150;
    Event *best_e = nullptr;
    for (int32_t i = m_currentEventIndex; i < m_eventsByStartTime.size(); ++i) {
      auto &e = m_eventsByStartTime[i];
      int32_t delta = e.m_timeStartMs - m_playBackTime;
      if (delta > 150) {
        if (best_e) {
          best_e->m_userHit = true;
          float waitedTime = m_timeWaited;
          m_timeWaited = 0;
          return {best, int32_t(waitedTime)};
        }
        return {best, 0};
      }
      if (e.m_midiKey == midiKey && !e.m_userHit) {
        if (std::abs(delta) < std::abs(best)) {
          best = delta;
          best_e = &e;
        }
      }
    }

    if (best_e) {
      best_e->m_userHit = true;
      float waitedTime = m_timeWaited;
      m_timeWaited = 0;
      return {best, int32_t(waitedTime)};
    }
    return {best, 0};
  }

  void recording_start_event(int32_t midiKey, int32_t velocity) {
    Event event;
    event.m_midiKey = midiKey;
    event.m_velocity = velocity;
    event.m_timeStartMs = m_playBackTime;
    m_eventsRecording.emplace_back(event);
  }

  void recording_end_event(int32_t midiKey) {
    int32_t timeMs = m_playBackTime;
    for (size_t i = 0; i < m_eventsRecording.size(); ++i) {
      if (m_eventsRecording[i].m_midiKey == midiKey) {
        m_eventsRecording[i].m_timeEndMs = timeMs;
        m_eventsByStartTime.emplace_back(m_eventsRecording[i]);
        m_eventsRecording.erase(m_eventsRecording.begin() + i);
        return;
      }
    }
  }

  void normalize() {
    sort_events();

    if (m_eventsByStartTime.empty())
      return;

    int32_t firstNoteTime = m_eventsByStartTime.front().m_timeStartMs;
    int32_t desiredFirstNoteTime = 2000; // configurable
    int32_t delta = desiredFirstNoteTime - firstNoteTime;
    for (auto &e : m_eventsByStartTime) {
      e.m_timeStartMs += delta;
      e.m_timeEndMs += delta;
    }
  }

  void sort_events() {
    std::sort(m_eventsByStartTime.begin(), m_eventsByStartTime.end(),
              [](const Event &a, const Event &b) {
                return a.m_timeStartMs < b.m_timeStartMs;
              });
  }

  void tick_time() {
    m_playBackTime += m_playBackTimeScale * 1000.0f *
                      m_recordingTimer.time_since_last_access_seconds_float() *
                      !m_paused;

    if (m_currentEventIndex >= m_eventsByStartTime.size())
      return;

    if (!m_wait) {
      if (m_playBackTime -
              m_eventsByStartTime[m_currentEventIndex].m_timeStartMs >
          150) {
        ++m_currentEventIndex;
        logmsg("note missed! current index %d", m_currentEventIndex);
        // MISSED NOTE - score / visuals
      }
    }
  }

  void limit_time_wait() {
    int32_t firstUnHit = 10000000;
    for (auto &e : m_eventsByStartTime) {
      if (!e.m_userHit && e.m_timeStartMs < firstUnHit) {
        firstUnHit = e.m_timeStartMs;
      }
    }

    if (m_playBackTime > firstUnHit) {
      m_timeWaited += m_playBackTime - firstUnHit;
      m_playBackTime = firstUnHit;
    }
  }

  bool m_paused = false;
  bool m_wait = true;

  float m_playBackTimeScale = 1.0f;
  float m_playBackTime = 0;
  float m_timeWaited = 0;
  int32_t m_currentEventIndex = 0;

  std::vector<Event> m_eventsByStartTime;
  std::vector<Event> m_eventsRecording;
  rynx::timer m_recordingTimer;

private:
};

namespace rynx::serialization {
template <> struct Serialize<Recording::Event> {
  template <typename IOStream>
  void serialize(const Recording::Event &t, IOStream &writer) {
    rynx::serialize(t.m_midiKey, writer);
    rynx::serialize(t.m_timeEndMs, writer);
    rynx::serialize(t.m_timeStartMs, writer);
    rynx::serialize(t.m_velocity, writer);
  }

  template <typename IOStream>
  void deserialize(Recording::Event &t, IOStream &reader) {
    rynx::deserialize(t.m_midiKey, reader);
    rynx::deserialize(t.m_timeEndMs, reader);
    rynx::deserialize(t.m_timeStartMs, reader);
    rynx::deserialize(t.m_velocity, reader);
  }
};

template <> struct Serialize<Recording> {
  template <typename IOStream>
  void serialize(const Recording &t, IOStream &writer) {
    rynx::serialize(t.m_eventsByStartTime, writer);
  }

  template <typename IOStream>
  void deserialize(Recording &t, IOStream &reader) {
    t.m_eventsRecording.clear();
    t.m_eventsByStartTime.clear();
    t.m_currentEventIndex = 0;
    t.m_playBackTime = 0;
    rynx::deserialize(t.m_eventsByStartTime, reader);
  }
};
}

int main(int /* argc */, char ** /* argv */) {

  // uses this thread services of rynx, for example in cpu performance
  // profiling.
  rynx::this_thread::rynx_thread_raii rynx_thread_services_required_token;

  SampleApplication application;
  application.openWindow(1920, 1080);
  application.startup_load();
  application.set_default_frame_processing();

  rynx::scheduler::task_scheduler &scheduler = application.scheduler();
  rynx::application::simulation &base_simulation = application.simulation();
  rynx::ecs &ecs = *base_simulation.m_ecs;

  application.set_resources();
  application.set_simulation_rules();

  rynx::mapped_input &gameInput =
      application.simulation_context()->get_resource<rynx::mapped_input>();
  rynx::observer_ptr<rynx::camera> camera =
      application.simulation_context()->get_resource_ptr<rynx::camera>();
  camera->setProjection(0.02f, 20000.0f, application.aspectRatio());
  camera->setPosition({0, 0, 300});
  camera->setDirection({0, 0, -1});
  camera->tick(1.0f);

  auto midiInputDeviceKeepaliveToken = gameInput.listenToMidiDeviceByIndex(0);

  // setup some debug controls

  auto menuCamera = rynx::make_shared<rynx::camera>();
  menuCamera->setPosition({0, 0, 1});
  menuCamera->setDirection({0, 0, -1});
  menuCamera->setUpVector({0, 1, 0});
  menuCamera->tick(1.0f);

  // TODO: Move under editor ruleset? or somewhere
  auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
  auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
  auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
  auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");

  auto zoomIn = gameInput.generateAndBindGameKey('U', "zoomIn");
  auto zoomOut = gameInput.generateAndBindGameKey('O', "zoomOut");

  // auto zoomIn =
  // gameInput.generateAndBindGameKey(gameInput.getMidiKeyPhysical(60),
  // "zoomIn"); auto zoomOut =
  // gameInput.generateAndBindGameKey(gameInput.getMidiKeyPhysical(61),
  // "zoomOut");

  // TODO - MOVE AUDIO SETUP SOMEWHERE ELSE
  // setup sound system
  rynx::sound::audio_system audio;

  // uint32_t soundIndex = audio.load("test.ogg");
  rynx::sound::configuration config;
  audio.open_output_device(64, 256);

  // TODO
  application.rendering_steps()->debug_draw_binary_config(
      application.get_binary_config_is_editor_running());

  auto camera_orientation_key = gameInput.generateAndBindGameKey(
      gameInput.getMouseKeyPhysical(1), "camera_orientation");

  rynx::timer dt_timer;
  float dt = 1.0f / 120.0f;

  /*
  if (gameInput.isKeyPressed(cameraUp)) {
          config = audio.play_sound(soundIndex, rynx::vec3f(), rynx::vec3f());
  }
  */

  // CREATE PIANO

  struct PianoGameData {
    struct Meshes {
      rynx::graphics::mesh_id m_whiteKey;
      rynx::graphics::mesh_id m_blackKey;
      rynx::graphics::mesh_id m_recording;
    };

    Meshes m_meshes;

    float m_recordingViewScale = 20;
  };

  PianoGameData data;

  data.m_meshes.m_recording =
      application.renderer().meshes()->create_transient_boundary(
          rynx::Shape::makeRectangle(1, 1), 0.1f);

  {
    application.reflection().create<int>();
    application.reflection().create<float>();

    float piano_scale = 50.0f;
    float x_progress = -2.2f * piano_scale;
    float white_width = 0.077f * piano_scale;
    float black_width = white_width * 0.5f;
    float black_offset_y = 0.15f * piano_scale;

    data.m_meshes.m_whiteKey =
        application.renderer().meshes()->create_transient(
            rynx::Shape::makeRectangle(2.0f * piano_scale,
                                       10.0f * piano_scale));

    data.m_meshes.m_blackKey =
        application.renderer().meshes()->create_transient(
            rynx::Shape::makeRectangle(1.0f * piano_scale, 7.0f * piano_scale));

    for (int i = 21; i < 120; ++i) {

      if (is_white_key(i)) {
        ecs.create(
            rynx::components::transform::matrix{},
            rynx::components::graphics::color{},
            rynx::components::transform::radius{piano_scale * 0.2f},
            rynx::components::transform::position{rynx::vec3f{
                x_progress + white_width * 0.5f, -piano_scale * 0.2f, 0.0f}},
            rynx::components::graphics::mesh{data.m_meshes.m_whiteKey},
            rynx::components::graphics::texture{"piano_white_key"}, i);

        x_offsets[i] = x_progress + white_width * 0.5f;
        x_progress += white_width;
      } else {
        ecs.create(
            rynx::components::transform::matrix{},
            rynx::components::graphics::color{},
            rynx::components::transform::radius{piano_scale * 0.15f},
            rynx::components::transform::position{rynx::vec3f{
                x_progress, 0.065f * piano_scale - piano_scale * 0.2f, 0.01f}},
            rynx::components::graphics::mesh{data.m_meshes.m_blackKey},
            rynx::components::graphics::texture{"piano_white_key"}, i);

        x_offsets[i] = x_progress;
      }
    }
  }

  class PianoGame {
  public:
    bool is_recording() const { return m_state == State::Recording; }

    bool is_play_wait() const { return m_state == State::PlayWait; }

    bool is_play_eval() const { return m_state == State::PlayEval; }

    void tick_time() {
      m_activeRecording.tick_time();
      if (is_play_wait())
        m_activeRecording.limit_time_wait();
    }

    // private:
    enum class State { Recording, PlayWait, PlayEval };

    bool start_playback_mode(rynx::ecs &ecs, rynx::graphics::texture_id tex_id,
                             float lookAheadTimeScale,
                             rynx::graphics::mesh_id blockMeshId) {
      stop_recording();

      m_score.reset();

      m_activeRecording.normalize();
      m_activeRecording.m_recordingTimer.reset();
      m_activeRecording.m_playBackTime = 0;
      m_activeRecording.m_currentEventIndex = 0;

      // first clear any existing track
      auto ids = ecs.query().in<float>().ids();
      for (auto id : ids) {
        ecs.attachToEntity(id, rynx::components::entity::dead());
      }

      for (auto &e : m_activeRecording.m_eventsByStartTime) {
        rynx::matrix4 m;

        float y_start = e.m_timeStartMs / lookAheadTimeScale;
        float y_end = (e.m_timeEndMs + 1) / lookAheadTimeScale;
        float y_height = y_end - y_start;
        float y_offset = y_start + y_height * 0.5f;
        float color_m = 0.7f + is_white_key(e.m_midiKey) * 0.3f;
        e.m_ecsId = ecs.create(
            rynx::components::transform::motion{},
            rynx::components::transform::matrix{},
            rynx::components::graphics::color{
                rynx::floats4(color_m, color_m, color_m, 1.0f)},
            rynx::components::transform::radius{y_height * 0.5f},
            rynx::components::transform::scale(
                (1.3f + is_white_key(e.m_midiKey) * 1.2f) / (y_height * 0.5f),
                1.0f / 0.707f, 1.0f),
            rynx::components::transform::position{rynx::vec3f{
                x_offsets[e.m_midiKey], (y_end + y_start) * 0.5f, -0.01f}},
            rynx::components::graphics::mesh{blockMeshId},
            rynx::components::graphics::texture{"button"},
            float(y_start + (y_end - y_start) * 0.5f));
      }

      return true;
    }

    bool stop_recording() {
      if (m_state == State::Recording) {
        m_state = State::PlayWait;
        m_activeRecording.m_eventsRecording.clear();
        return true;
      }
      return false;
    }

    bool start_recording() {
      if (m_state != State::Recording) {
        m_state = State::Recording;
        m_activeRecording.m_eventsByStartTime.clear();
        m_activeRecording.m_recordingTimer.reset();
        m_activeRecording.m_playBackTime = 0;
        m_activeRecording.m_currentEventIndex = 0;
        return true;
      }
      return false;
    }

    void add_score(float timeDelta, float waitTime) {
      m_score.m_waitError += waitTime * waitTime;
      m_score.m_hittingScore += 200 * 200 - timeDelta * timeDelta;

      //
    }

    struct Score {
      void reset() { *this = Score(); }

      float m_hittingScore = 0;
      float m_waitError = 0;

      int m_missedNotes = 0;
      int m_okNotes = 0;
      int m_goodNotes = 0;
      int m_excellentNotes = 0;
      int m_perfectNotes = 0;
    };

    Score m_score;

    Recording m_activeRecording;
    State m_state = State::Recording;
  };

  PianoGame pianoGame;

  application.simulation().m_vfs->mount().native_directory("../midi/",
                                                           "/midi/");
  application.simulation().m_vfs->mount().native_directory("../rynx-piano/",
                                                           "/piano/");

  auto &menuSystem =
      application.simulation().m_context->get_resource<rynx::menu::System>();
  auto *menuRoot = menuSystem.root();

  // pianogame menu root
  auto mainDiv = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{1, 1, 0},
                                                    rynx::vec3f{0, 0, 0});

  // create time scale slider
  auto tempoSlider = rynx::make_shared<rynx::menu::SlideBarVertical>(
      application.textures()->findTextureByName("piano_white_key"),
      application.textures()->findTextureByName("piano_black_key"),
      rynx::vec3f{0.25f, 0.04f, 0},
      0.0f, // min time scale = 0 = paused
      2.0f, // max time scale = 2*2 = 4x time
      1.0f  // default = 1x
  );

  tempoSlider->on_value_changed([&](float timeScale) {
    pianoGame.m_activeRecording.m_playBackTimeScale = timeScale * timeScale;
  });

  mainDiv->addChild(tempoSlider);
  menuRoot->addChild(mainDiv);

  // strictness slider

  // end screen (score)
  // show when play eval/wait mode

  // song select
  // ??

  // button for select midi input
  /*
  * 			rynx::graphics::texture_id texture,
                          vec3<float> scale,
                          vec3<float> position = vec3<float>(),
                          float frame_edge_percentage = 0.2f
  */
  auto selectMidiInput = rynx::make_shared<rynx::menu::Button>(
      application.textures()->findTextureByName("frame"),
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f(), 0.2f);

  selectMidiInput->text().text("input device");
  selectMidiInput->on_click([&]() {
    auto devices = rynx::io::midi::list_devices();
    auto midi_input_select = rynx::make_shared<rynx::menu::ListSelector>(
        application.textures()->findTextureByName("frame"),
        rynx::vec3f(0.5f, 0.5f, 0.0f));

    for (auto &dev : devices) {
      midi_input_select->category_setup().add_option(dev);
    }

    menuSystem.push_popup(midi_input_select);
    midi_input_select->display([&](rynx::string str) {
      menuSystem.execute([&, str]() {
        auto devs = rynx::io::midi::list_devices();
        for (size_t i = 0; i < devs.size(); ++i) {
          if (devs[i] == str) {
            midiInputDeviceKeepaliveToken =
                gameInput.listenToMidiDeviceByIndex(i);
          }
        }

        menuSystem.pop_popup();
      });
    });
  });

  menuRoot->addChild(selectMidiInput);

  auto openMidiFile = rynx::make_shared<rynx::menu::Button>(
      application.textures()->findTextureByName("frame"),
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f(), 0.2f);

  openMidiFile->text().text("import midi");
  openMidiFile->on_click([&]() {
    auto file_selector = rynx::make_shared<rynx::menu::FileSelector>(
        *application.simulation().m_vfs,
        application.textures()->findTextureByName("frame"),
        rynx::vec3<float>(0.5f, 0.5f, 0.0f), rynx::vec3<float>());

    menuSystem.push_popup(file_selector);
    file_selector->display(
        "/midi/",
        [&](rynx::string filePath) {
          menuSystem.execute([&, filePath]() {
            // pianoGame.load_midi_file(*application.simulation().m_vfs,
            // filePath);
          });
          menuSystem.pop_popup();
        },
        [](rynx::string dirPath) {});
  });

  auto loadPianoFile = rynx::make_shared<rynx::menu::Button>(
      application.textures()->findTextureByName("frame"),
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f(), 0.2f);

  loadPianoFile->text().text("Load");
  loadPianoFile->on_click([&]() {
    auto file_selector = rynx::make_shared<rynx::menu::FileSelector>(
        *application.simulation().m_vfs,
        application.textures()->findTextureByName("frame"),
        rynx::vec3<float>(0.5f, 0.5f, 0.0f), rynx::vec3<float>());

    menuSystem.push_popup(file_selector);
    file_selector->display(
        "/piano/",
        [&](rynx::string filePath) {
          menuSystem.execute([&, filePath]() {
            auto pianoFile =
                application.simulation().m_vfs->open_read(filePath);
            auto serializedData = pianoFile->read_all();
            rynx::serialization::vector_reader reader(
                std::move(serializedData));
            rynx::deserialize(pianoGame.m_activeRecording, reader);
            pianoGame.start_playback_mode(
                ecs, application.textures()->findTextureByName("frame"),
                data.m_recordingViewScale, data.m_meshes.m_recording);
          });
          menuSystem.pop_popup();
        },
        [](rynx::string dirPath) {});
  });

  auto savePianoFile = rynx::make_shared<rynx::menu::Button>(
      application.textures()->findTextureByName("frame"),
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f(), 0.2f);

  savePianoFile->text().text("Save");
  savePianoFile->on_click([&]() {
    auto file_selector = rynx::make_shared<rynx::menu::FileSelector>(
        *application.simulation().m_vfs,
        application.textures()->findTextureByName("frame"),
        rynx::vec3<float>(0.5f, 0.5f, 0.0f), rynx::vec3<float>());

    file_selector->configure().m_allowNewFile = true;
    file_selector->configure().m_allowNewDir = true;
    file_selector->configure().m_addFileExtensionToNewFiles = ".rynxpiano";

    menuSystem.push_popup(file_selector);
    file_selector->display(
        "/piano/",
        [&](rynx::string filePath) {
          menuSystem.execute([&, filePath]() {
            rynx::serialization::vector_writer writer;
            rynx::serialize(pianoGame.m_activeRecording, writer);
            auto pianoFile =
                application.simulation().m_vfs->open_write(filePath);
            pianoFile->write(writer.data().data(), writer.data().size());
            std::cout << "saved!" << std::endl;
          });
          menuSystem.pop_popup();
        },
        [](rynx::string dirPath) {});
  });

  auto recordModeButton = rynx::make_shared<rynx::menu::Button>(
      application.textures()->findTextureByName("frame"),
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f(), 0.2f);

  recordModeButton->text().text("rec");
  recordModeButton->text().color({1.0f, 0.1f, 0.1f, 1.0f});
  recordModeButton->on_click([&recordModeButton, &pianoGame]() {
    if (pianoGame.m_state == PianoGame::State::Recording) {
      pianoGame.m_state = PianoGame::State::PlayWait;
      recordModeButton->text().text("play");
      recordModeButton->text().color({0.1f, 1.0f, 0.1f, 1.0f});
    } else {
      pianoGame.m_state = PianoGame::State::Recording;
      recordModeButton->text().text("rec");
      recordModeButton->text().color({1.0f, 0.1f, 0.1f, 1.0f});
    }
  });

  auto scoreLabel = rynx::make_shared<rynx::menu::Text>(
      rynx::vec3f(0.25f, 0.025f, 1.0f), rynx::vec3f());

  menuRoot->addChild(recordModeButton);
  menuRoot->addChild(scoreLabel);

  menuRoot->addChild(openMidiFile);
  menuRoot->addChild(savePianoFile);
  menuRoot->addChild(loadPianoFile);

  tempoSlider->align().top_right_inside();
  openMidiFile->align()
      .target(tempoSlider.get())
      .bottom_outside()
      .right_inside();
  selectMidiInput->align()
      .target(openMidiFile.get())
      .bottom_outside()
      .right_inside();
  savePianoFile->align()
      .target(selectMidiInput.get())
      .bottom_outside()
      .right_inside();
  loadPianoFile->align()
      .target(savePianoFile.get())
      .bottom_outside()
      .right_inside();

  recordModeButton->align()
      .target(tempoSlider.get())
      .left_outside()
      .top_inside();

  scoreLabel->align()
      .target(recordModeButton.get())
      .left_outside()
      .top_inside();
  scoreLabel->target_scale() = rynx::vec3f{0.25f, 0.025f, 1.0f};
  scoreLabel->text_config().allow_input = false;

  // enter record mode
  // save recording to file

  // TODO: Main loop should probably be implemented under application?
  //       User should not need to worry about logic/render frame rate
  //       decouplings.
  while (!application.isExitRequested()) {

    dt =
        std::min(0.016f, std::max(0.001f, dt_timer.time_since_last_access_ms() *
                                              0.001f));
    dt_timer.time_since_begin_ms();

    rynx_profile("Main", "frame");

    auto mousePos = application.input()->getCursorPosition();

    {
      rynx_profile("Main", "update camera");

      if (gameInput.isKeyDown(camera_orientation_key)) {
        camera->rotate(gameInput.mouseDelta());
      }

      camera->tick(3.0f * dt);
      camera->rebuild_view_matrix();
    }

    application.user().frame_begin();
    application.user().menu_input();
    application.user().logic_frame_start();
    application.user().logic_frame_wait_end();
    application.user().menu_frame_execute();
    application.user().wait_gpu_done_and_swap();
    application.user().render();
    application.user().frame_end();

    // menu input must happen first before tick. in case menu components
    // reserve some input as private.

    pianoGame.tick_time();

    scoreLabel->text(rynx::to_string(pianoGame.m_score.m_hittingScore));

    ecs.query().for_each(
        [&](float originalY, rynx::components::transform::position &p) {
          p.value.y = originalY - pianoGame.m_activeRecording.m_playBackTime /
                                      data.m_recordingViewScale;
        });

    ecs.query().for_each([&](int midiKey,
                             rynx::components::graphics::color &c) {
      if (pianoGame.is_play_eval() || pianoGame.is_play_wait()) {
        if (gameInput.isKeyPressed(gameInput.getMidiKeyPhysical(midiKey))) {
          auto [timingDelta, waitTime] =
              pianoGame.m_activeRecording.find_nearest_note_start(midiKey);

          pianoGame.add_score(timingDelta, waitTime);
          scoreLabel->scale_local(rynx::vec3f{0.25f, 0.045f, 1.0f});
          scoreLabel->target_scale() = rynx::vec3f{0.25f, 0.025f, 1.0f};

          rynx::floats4 success_color{0.15f, 0.7f, 0.15f, 1.0f};
          rynx::floats4 error_color{0.9f, 0.2f, 0.2f, 1.0f};
          if (std::abs(timingDelta) < 50) {
            c.value = success_color;
          } else if (std::abs(timingDelta) > 150) {
            c.value = error_color;
          } else {
            float a = (std::abs(timingDelta) - 50.0f) / 100.0f;
            c.value = success_color * (1.0f - a) + error_color * a;
          }
        } else {
          float v = 0.10f + is_white_key(midiKey) * 0.7f;
          rynx::floats4 targetColor{v, v, v, 1.0f};
          c.value = c.value * (1.0f - dt * 3) + targetColor * dt * 3;
        }
      } else {
        if (gameInput.isKeyDown(gameInput.getMidiKeyPhysical(midiKey))) {
          c.value = rynx::floats4{0.15f, 0.7f, 0.15f, 1.0f};
        } else {
          float v = 0.10f + is_white_key(midiKey) * 0.7f;
          rynx::floats4 targetColor{v, v, v, 1.0f};
          c.value = c.value * (1.0f - dt * 3) + targetColor * dt * 3;
        }
      }

      if (pianoGame.is_recording()) {
        if (gameInput.isKeyPressed(gameInput.getMidiKeyPhysical(midiKey))) {
          pianoGame.m_activeRecording.recording_start_event(midiKey, 0);
        }
        if (gameInput.isKeyReleased(gameInput.getMidiKeyPhysical(midiKey))) {
          pianoGame.m_activeRecording.recording_end_event(midiKey);
        }
      }
    });

    if (gameInput.isKeyClicked(rynx::key::physical('B'))) {
      ecs.erase(ecs.query().in<float>().ids());
      pianoGame.start_recording();
    }
    if (gameInput.isKeyClicked(rynx::key::physical('N'))) {
      pianoGame.stop_recording();
      pianoGame.start_playback_mode(
          ecs, application.textures()->findTextureByName("frame"),
          data.m_recordingViewScale, data.m_meshes.m_recording);
    }

    {
      // TODO: Input handling should probably not be here.
      {
        // TODO: inhibit menu dedicated inputs
        const float camera_translate_multiplier = 400.4f * dt;
        const float camera_zoom_multiplier = (1.0f - dt);
        if (gameInput.isKeyDown(cameraUp)) {
          camera->translate(camera->local_up() * camera_translate_multiplier);
        }
        if (gameInput.isKeyDown(cameraLeft)) {
          camera->translate(camera->local_left() * camera_translate_multiplier);
        }
        if (gameInput.isKeyDown(cameraRight)) {
          camera->translate(-camera->local_left() *
                            camera_translate_multiplier);
        }
        if (gameInput.isKeyDown(cameraDown)) {
          camera->translate(-camera->local_up() * camera_translate_multiplier);
        }
        if (gameInput.isKeyDown(zoomOut)) {
          camera->translate(
              camera->position() *
              rynx::vec3f{0, 0, 1.0f - 1.0f * camera_zoom_multiplier});
        }
        if (gameInput.isKeyDown(zoomIn)) {
          camera->translate(
              camera->position() *
              rynx::vec3f{0, 0, 1.0f - 1.0f / camera_zoom_multiplier});
        }
      }
      if (gameInput.isKeyClicked(rynx::key::physical('X'))) {
        rynx::profiling::write_profile_log();
      }
    }
  }

  {
    rynx_profile("Main", "Clean up dead entitites");
    auto ids_dead = ecs.query().in<rynx::components::entity::dead>().ids();
    base_simulation.m_logic.entities_erased(*base_simulation.m_context,
                                            ids_dead);
    ecs.erase(ids_dead);
  }

  // lets avoid using all computing power for now.
  // TODO: Proper time tracking for frames.
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

return 0;
}
