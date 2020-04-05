#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/audio/dsp/signal.hpp>

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

            void resample(float multiplier);
        };

        struct effect_config {
            float pitch_shift = 0.0f;
            float tempo_shift = 0.0f;
        };

        struct source_instance {
            source_instance();

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
            
            float completion_rate() const;
            bool is_active() const;
        };

        // TODO: Should enforce that all loads are done before output is opened.
        class audio_system {
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

        public:
            // Not allowed to move or copy or jack shit. This has to stay in place in memory, because of the audiorender thread.
            audio_system(audio_system&&) = delete;
            audio_system(const audio_system&) = delete;
            audio_system& operator=(audio_system&&) = delete;
            audio_system& operator=(const audio_system&) = delete;

            size_t num_samples_in_buffer(int bufferIndex) {
                return m_soundBank[bufferIndex].left.size();
            }

            audio_system();
            ~audio_system();
            
            uint32_t load(std::string path); // all loads should be complete before opening output device.
            configuration play_sound(int soundIndex, vec3f position, vec3f direction, float loudness = 1.0f);
            void open_output_device(int numChannels = 64, int samplesPerRender = 256);
            
            void render_audio(float* outBuf, size_t numSamples); // Do not call this. This is called automatically.
        };
    }
}