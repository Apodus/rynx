#pragma once

#include <rynx/audio/dsp/signal.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/random.hpp>

#include <vector>
#include <cstdint>

#include <memory>
#include <atomic>
#include <thread>
#include <string>

namespace rynx {
    namespace sound {
        class audio_system;

        struct buffer {
            std::vector<float> left;
            std::vector<float> right;
            
            float sampleRate = 0;
            void resample(float multiplier);
        };

        struct effect_config {
            float pitch_shift = 0.0f;
            float tempo_shift = 0.0f;
        };

        struct source_instance {
            source_instance();

            struct {
                float linear = 0.0f;
                float quadratic = 1.0f;
            } m_attenuation;

            vec3f m_position;
            vec3f m_direction;
            uint32_t m_bufferIndex = 0;
            uint32_t m_sampleIndex = 0;
            uint32_t m_soundCounter = 0;
            float m_loudness = 1.0f;

            effect_config m_effects;

            std::vector<float> prev_left;
            std::vector<float> prev_right;
        };

        struct configuration {
        private:
            audio_system* m_rynxAudio = nullptr;
            source_instance* m_soundData = nullptr;
            uint32_t m_soundCounter = 0;

        public:
            configuration() = default;
            configuration(audio_system& audio, source_instance& sound);
            configuration& set_position(vec3f pos);
            configuration& set_direction(vec3f dir);
            configuration& set_loudness(float loudness);
            
            configuration& set_pitch_shift(float octaves);
            configuration& set_tempo_shift(float tempo_multiplier);

            configuration& set_attenuation_quadratic(float v);
            configuration& set_attenuation_linear(float v);

            float completion_rate() const;
            bool is_active() const;
        };

        // TODO: Should enforce that all loads are done before output is opened.
        class audio_system {
        public:
            enum class format {
                int16,
                int32,
                float32,
                undefined
            };

            class named_events {
            public:
                void insert(const std::string& name, int value) {
                    m_data[name].emplace_back(value);
                }

                int get(const std::string& name) const {
                    auto it = m_data.find(name);
                    if (it != m_data.end()) {
                        if (!it->second.empty()) {
                            return it->second[m_random(it->second.size())];
                        }
                    }
                    return 0;
                }

            private:
                mutable rynx::math::rand64 m_random;
                rynx::unordered_map<std::string, std::vector<int>> m_data;
            };

        private:
            std::vector<buffer> m_soundBank;
            std::vector<std::unique_ptr<std::atomic<uint32_t>>> m_channels; // Three states. 0 = free, 1 = reserved, 2 = ready for playback?
            std::vector<source_instance> m_channelDatas;
            source_instance m_invalidChannel;

            vec3f m_listenerPosition;
            vec3f m_listenerLeft;
            vec3f m_listenerUp;
            vec3f m_listenerForward;
            void* stream = nullptr; // portaudio stream
            void setNumChannels(int channels);

            float m_globalVolume = 1.0f;
            float m_volumeScaleCurrent = 1.0f;
            float m_currentSampleRate = 48000.0f;

            float m_default_quadratic_attenuation = 1.0f;
            float m_default_linear_attenuation = 0.0f;

            std::unique_ptr<float[]> m_outBuf;
            size_t m_outBufLength = 0;
            format m_outputFormat = format::undefined;
            
            named_events m_namedEvents;

        public:
            audio_system(audio_system&&) = delete;
            audio_system(const audio_system&) = delete;
            audio_system& operator=(audio_system&&) = delete;
            audio_system& operator=(const audio_system&) = delete;

            size_t num_samples_in_buffer(int bufferIndex) {
                return m_soundBank[bufferIndex].left.size();
            }

            audio_system& set_default_attentuation_quadratic(float v) { m_default_quadratic_attenuation = v; return *this; }
            audio_system& set_default_attentuation_linear(float v) { m_default_linear_attenuation = v; return *this; }

            audio_system& set_volume(float v) { m_globalVolume = v; return *this; }
            audio_system& adjust_volume(float v) { m_globalVolume *= v; return *this; }

            audio_system& set_listener_position(rynx::vec3f pos) { m_listenerPosition = pos; return *this; }

            audio_system();
            ~audio_system();
            
            uint32_t load(std::string path); // returns the id of the loaded sound buffer.
            audio_system& load(std::string path, std::string event_name); // load sound as a named event. use the event name later to refer to it.

            configuration play_sound(int bufferId, vec3f position, vec3f direction = vec3f(), float loudness = 1.0f);
            configuration play_sound(const std::string& named_event, vec3f position, vec3f direction = vec3f(), float loudness = 1.0f);
            
            void open_output_device(int numChannels = 64, int samplesPerRender = 256, audio_system::format format = format::int16);
            void render_audio(void* outBuf, size_t numSamples); // Do not call this. This is called automatically.
        };
    }
}