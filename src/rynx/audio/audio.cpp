
#include <portaudio.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#include <rynx/math/vector.hpp>
#include <rynx/audio/audio.hpp>

#include <rynx/audio/dsp/signal.hpp>

#include <atomic>
#include <array>
#include <vector>

#include <thread>
#include <chrono>

#include <fstream>

#pragma optimize("s", on)
#pragma optimize("g", on)

static int audio_callback(const void* /* inputBuffer */, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* /* timeInfo */,
    PaStreamCallbackFlags /* statusFlags */,
    void* userData)
{
    static_cast<rynx::sound::audio_system*>(userData)->render_audio(static_cast<float*>(outputBuffer), framesPerBuffer);
    return 0;
}

static constexpr int fft_window_size = 2048;

namespace {
    struct ogg_file
    {
        char* curPtr;
        char* filePtr;
        size_t fileSize;
    };

    size_t AR_readOgg(void* dst, size_t size1, size_t size2, void* fh)
    {
        ogg_file* of = reinterpret_cast<ogg_file*>(fh);
        size_t len = size1 * size2;
        if (of->curPtr + len > of->filePtr + of->fileSize)
        {
            len = of->filePtr + of->fileSize - of->curPtr;
        }
        memcpy(dst, of->curPtr, len);
        of->curPtr += len;
        return len;
    }

    int AR_seekOgg(void* fh, ogg_int64_t to, int type) {
        ogg_file* of = reinterpret_cast<ogg_file*>(fh);

        switch (type) {
        case SEEK_CUR:
            of->curPtr += to;
            break;
        case SEEK_END:
            of->curPtr = of->filePtr + of->fileSize - to;
            break;
        case SEEK_SET:
            of->curPtr = of->filePtr + to;
            break;
        default:
            return -1;
        }
        if (of->curPtr < of->filePtr) {
            of->curPtr = of->filePtr;
            return -1;
        }
        if (of->curPtr > of->filePtr + of->fileSize) {
            of->curPtr = of->filePtr + of->fileSize;
            return -1;
        }
        return 0;
    }

    int AR_closeOgg(void*)
    {
        return 0;
    }

    long AR_tellOgg(void* fh)
    {
        ogg_file* of = reinterpret_cast<ogg_file*>(fh);
        return static_cast<long>(of->curPtr - of->filePtr);
    }
}

void rynx::sound::buffer::resample(float multiplier) {
    buffer other;
    other.left.resize(static_cast<size_t>(left.size() * multiplier), 0.0f);
    other.right.resize(static_cast<size_t>(right.size() * multiplier), 0.0f);
    other.sampleRate = sampleRate * multiplier;

    float inv_mul = 1.0f / multiplier;
    for (size_t i = 0; i < other.left.size(); ++i) {
        float src_index = i * inv_mul;
        size_t src_min = size_t(src_index);
        size_t src_max = size_t(src_index) + 1;

        float p_min = 1.0f - (src_index - src_min);
        float p_max = 1.0f - p_min;

        if (src_min < left.size()) {
            other.left[i] += left[src_min] * p_min;
            other.right[i] += right[src_min] * p_min;
        }

        if (src_max < left.size()) {
            other.left[i] += left[src_max] * p_max;
            other.right[i] += right[src_max] * p_max;
        }
    }

    *this = std::move(other);
}

rynx::sound::buffer loadOggVorbis(std::string path, int target_sample_rate = 44100) {
    std::ifstream in(path, std::ios::binary);
    in.seekg(0, std::ios::ios_base::end);
    auto size = in.tellg();
    in.seekg(0, std::ios::ios_base::beg);
    std::vector<char> buf(size);
    in.read(buf.data(), size);

    ogg_file t;
    t.curPtr = t.filePtr = buf.data();
    t.fileSize = buf.size();

    OggVorbis_File ov;
    memset(&ov, 0, sizeof(OggVorbis_File));

    ov_callbacks callbacks;
    callbacks.read_func = AR_readOgg;
    callbacks.seek_func = AR_seekOgg;
    callbacks.close_func = AR_closeOgg;
    callbacks.tell_func = AR_tellOgg;

    int open_return = ov_open_callbacks((void*)&t, &ov, NULL, -1, callbacks);

    vorbis_info* vi = ov_info(&ov, -1);
    rynx_assert(vi, "loading ogg failed");

    int num_channels = vi->channels;
    int sample_rate = vi->rate;
    int64_t total_samples = ov_pcm_total(&ov, -1);
    
    rynx::sound::buffer buffer;
    buffer.left.resize(total_samples, 0.0f);
    buffer.right.resize(total_samples, 0.0f);
    buffer.sampleRate = float(sample_rate);

    long samples_read = 0;
    int current_section = 0;

    while (true) {
        float** pcm;
        long ret = ov_read_float(&ov, &pcm, static_cast<int>(total_samples), &current_section);
        if (ret == 0) {
            logmsg("loaded ogg with %d samples: '%s'", samples_read, path.c_str());
            break;
        }
        else if (ret < 0) {
            logmsg("failed to load: '%s'", path.c_str());
        }
        else {
            // successfully got some data. copy it to our buffer.
            if (num_channels >= 2) {
                for (long i = 0; i < ret; ++i) {
                    buffer.left[samples_read + i] = pcm[0][i];
                    buffer.right[samples_read + i] = pcm[1][i];
                }
            }
            else if (num_channels == 1) {
                for (long i = 0; i < ret; ++i) {
                    buffer.left[samples_read + i] = pcm[0][i];
                    buffer.right[samples_read + i] = pcm[0][i];
                }
            }
            samples_read += ret;
        }
    }

    ov_clear(&ov);

    float resample_index_mul = float(target_sample_rate) / float(sample_rate);
    buffer.resample(resample_index_mul);

    return buffer;
}

rynx::sound::source_instance::source_instance() {
    prev_left.resize(fft_window_size);
    prev_right.resize(fft_window_size);
}

rynx::sound::configuration::configuration(audio_system& audio, source_instance& sound) {
    m_rynxAudio = &audio;
    m_soundData = &sound;
    m_soundCounter = sound.m_soundCounter;
}

rynx::sound::configuration& rynx::sound::configuration::set_position(vec3f pos) {
    if (is_active()) {
        m_soundData->m_position = pos;
    }
    return *this;
}

rynx::sound::configuration& rynx::sound::configuration::set_direction(vec3f dir) {
    if (is_active()) {
        m_soundData->m_direction = dir;
    }
    return *this;
}

rynx::sound::configuration& rynx::sound::configuration::set_loudness(float loudness) {
    if (is_active()) {
        m_soundData->m_loudness = loudness;
    }
    return *this;
}

rynx::sound::configuration& rynx::sound::configuration::set_pitch_shift(float octaves) {
    if (is_active()) {
        m_soundData->m_effects.pitch_shift = octaves;
    }
    return *this;
}


float rynx::sound::configuration::completion_rate() const {
    uint32_t sample = m_soundData->m_sampleIndex;
    uint32_t buffer = m_soundData->m_bufferIndex;
    if (is_active()) {
        return float(sample) / float(m_rynxAudio->num_samples_in_buffer(buffer) + 1);
    }
    return 1;
}

bool rynx::sound::configuration::is_active() const {
    rynx_assert(m_soundData != nullptr, "sound configuration object invalid");
    rynx_assert(m_rynxAudio != nullptr, "sound configuration object invalid");
    return m_soundCounter == m_soundData->m_soundCounter;
}



rynx::sound::audio_system::audio_system() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "failed to initialize audio system: " << std::string(Pa_GetErrorText(err)) << std::endl;
    }
    m_soundBank.emplace_back(); // guarantee that sound index zero points to a silent (and empty) sample.
}

rynx::sound::audio_system::~audio_system() {
    PaError err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "failed to close audio stream: " << std::string(Pa_GetErrorText(err)) << std::endl;
    }

    err = Pa_Terminate();
    if (err != paNoError) {
        std::cerr << "failed to shutdown audio system: " << std::string(Pa_GetErrorText(err)) << std::endl;
    }
}

void rynx::sound::audio_system::setNumChannels(int channels) {
    m_channelDatas.resize(channels);
    m_channels.resize(channels);
    for (size_t i = 0; i < channels; ++i) {
        m_channels[i] = std::make_unique<std::atomic<uint32_t>>(0);
    }
}

uint32_t rynx::sound::audio_system::load(std::string path) {
    uint32_t soundIndex = static_cast<uint32_t>(m_soundBank.size());
    m_soundBank.emplace_back(loadOggVorbis(path, 60000));
    return soundIndex;
}

rynx::sound::configuration rynx::sound::audio_system::play_sound(int soundIndex, vec3f position, vec3f direction, float loudness) {
    rynx_assert(soundIndex <= m_soundBank.size() && soundIndex >= 0, "out of bounds sound index!");
    rynx_assert(loudness >= 0.0f && loudness <= 10.0f, "really loud sound requested. is this intentional?");
    for (size_t i = 0; i < m_channels.size(); ++i) {
        uint32_t expected = 0;
        if (m_channels[i]->compare_exchange_weak(expected, 1u)) {
            m_channelDatas[i].m_bufferIndex = soundIndex;
            m_channelDatas[i].m_sampleIndex = 0;
            m_channelDatas[i].m_position = position;
            m_channelDatas[i].m_direction = direction;
            m_channelDatas[i].m_loudness = loudness;
            m_channels[i]->store(2);
            return rynx::sound::configuration(*this, m_channelDatas[i]);
        }
    }

    rynx::sound::configuration result(*this, m_invalidChannel);
    ++m_invalidChannel.m_soundCounter; // invalidate config immediately.
    return result;
}

void rynx::sound::audio_system::open_output_device(int numChannels, int samplesPerRender) {

    setNumChannels(numChannels);

    {
        m_currentSampleRate = static_cast<float>(Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultSampleRate);
        std::cerr << "sample rate for default device: " << m_currentSampleRate << std::endl;

        /* Open an audio I/O stream. */
        PaError err = Pa_OpenDefaultStream(&stream,
            0, // no input channels
            2, // stereo output
            paFloat32,
            m_currentSampleRate,
            samplesPerRender,
            audio_callback,
            this);

        if (err != paNoError) {
            std::cerr << "failed to open default audio output stream: " << std::string(Pa_GetErrorText(err)) << std::endl;
        }
    }

    {
        PaError err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "failed to start audio output stream: " << std::string(Pa_GetErrorText(err)) << std::endl;
        }
    }
}

void fill(std::vector<float>& dst, const float* src, size_t amount) {
    float* dst_p = dst.data();
    const float* end = dst_p + amount;
    while(dst_p != end)
        *dst_p++ = *src++;
}

std::vector<float> stretch(const float* src, size_t dataRemaining, size_t window, size_t targetLength) {
    std::vector<float> result(targetLength + window, 0.0f);

    if (targetLength > window) {
        std::vector<float> w(window, 0.0f);
        fill(w, src, std::min(dataRemaining, window));
        
        for (int i = 0; i < 2 * targetLength / window; ++i) {
            for (int k = 0; k < window; ++k) {
                float volume = float(2 * k) / float(window);
                if (volume > 1.0f) {
                    volume = 2.0f - volume;
                }

                result.data()[int32_t(i * window * 0.5f) + k] += src[k] * volume;
            }
        }
    }
    else {
        // ??
    }

    return result;
}

void rynx::sound::audio_system::render_audio(float* outBuf, size_t numSamples) {
    std::memset(outBuf, 0, sizeof(float) * numSamples * 2);
    for (size_t i = 0; i < m_channels.size(); ++i) {
        if (m_channels[i]->load() == 2) {
            source_instance& data = m_channelDatas[i];
            float distance = (data.m_position - m_listenerPosition).length_squared() + 10;
            float volume = 10.0f / distance;
            volume *= data.m_loudness;

            auto& sound = m_soundBank[data.m_bufferIndex];
            float resample_multiplier = sound.sampleRate / m_currentSampleRate;

            if (data.m_effects.pitch_shift != 0.0f || data.m_effects.tempo_shift != 0.0f) {
                size_t fft_window = 2 * numSamples;
                float multiplier = std::pow(2.0f, data.m_effects.pitch_shift);
                float tempo_mul = std::pow(2.0f, data.m_effects.tempo_shift) * resample_multiplier;

                multiplier *= tempo_mul;

                std::vector<float> chunk_left(fft_window, 0.0f);
                std::vector<float> chunk_right(fft_window, 0.0f);

                size_t num_samples_to_take = size_t((sound.left.size() - data.m_sampleIndex - 1) / multiplier);
                if (num_samples_to_take > fft_window)
                    num_samples_to_take = fft_window;
                    
                for (size_t k = 0; k < num_samples_to_take; ++k) {
                    float src_index_f = multiplier * k;
                    size_t src_index_prev = size_t(src_index_f);
                    size_t src_index_next = size_t(src_index_f) + 1;
                    float p_prev = 1.0f - (src_index_f - src_index_prev);
                    float p_next = 1.0f - p_prev;

                    chunk_left[k] = sound.left[src_index_prev + data.m_sampleIndex] * p_prev + sound.left[src_index_next + data.m_sampleIndex] * p_next;
                    chunk_right[k] = sound.right[src_index_prev + data.m_sampleIndex] * p_prev + sound.right[src_index_next + data.m_sampleIndex] * p_next;
                }

                float numSamples_f = float(numSamples);
                for (size_t k = 0; k < numSamples; ++k) {
                    float p = float(k) / numSamples_f;
                    outBuf[2 * k + 0] += volume * chunk_left[k] * p + data.prev_left[k + numSamples] * volume * (1.0f - p);
                    outBuf[2 * k + 1] += volume * chunk_right[k] * p + data.prev_right[k + numSamples] * volume * (1.0f - p);
                }

                data.prev_left = std::move(chunk_left);
                data.prev_right = std::move(chunk_right);

                data.m_sampleIndex += static_cast<uint32_t>(numSamples * tempo_mul);
            }
            else {
                for (size_t k = 0; (static_cast<int>(k * resample_multiplier) + data.m_sampleIndex + 1 < sound.left.size()) & (k < numSamples); ++k) {
                    float base = k * resample_multiplier;
                    int base_i = static_cast<int>(base);
                    size_t index1 = static_cast<int>(k * resample_multiplier);
                    size_t index2 = index1 + 1;

                    float mul1 = 1.0f - (base - base_i);
                    float mul2 = 1.0f - mul1;

                    outBuf[2 * k + 0] += volume * (sound.left[index1 + data.m_sampleIndex] * mul1 + sound.left[index2 + data.m_sampleIndex] * mul2);
                    outBuf[2 * k + 1] += volume * (sound.right[index1 + data.m_sampleIndex] * mul1 + sound.right[index2 + data.m_sampleIndex] * mul2);
                }
                data.m_sampleIndex += static_cast<uint32_t>(numSamples * resample_multiplier);
            }

            // sound playback complete.
            if (data.m_sampleIndex >= sound.left.size()) {
                data.m_sampleIndex = 0;
                data.m_bufferIndex = 0;
                data.m_loudness = 0;
                ++data.m_soundCounter;
                
                for (uint32_t k = 0; k < numSamples; ++k) {
                    data.prev_left[k + numSamples] = 0.0f;
                    data.prev_right[k + numSamples] = 0.0f;
                }

                m_channels[i]->store(0);
            }
        }
    }

    float max = 0;
    for (size_t i = 0; i < numSamples * 2; ++i) {
        float v = outBuf[i] * m_volumeScaleCurrent;
        v *= v;
        max = (v > max) ? v : max;
    }

    float scaleMultiplier = 1.04f - 0.08f * (max > 0.1f);
    m_volumeScaleCurrent *= scaleMultiplier;
    m_volumeScaleCurrent = std::min(m_volumeScaleCurrent, 1.0f);

    for (size_t i = 0; i < numSamples * 2; ++i) {
        float volume_i = outBuf[i] * m_volumeScaleCurrent;
        outBuf[i] = std::tan(volume_i * m_globalVolume); // lol mix hehe. quality.
    }
}

