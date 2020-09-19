
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
    static_cast<rynx::sound::audio_system*>(userData)->render_audio(outputBuffer, framesPerBuffer);
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

rynx::sound::configuration& rynx::sound::configuration::set_tempo_shift(float tempo_multiplier) {
    if (is_active()) {
        m_soundData->m_effects.tempo_shift = tempo_multiplier;
    }
    return *this;
}

rynx::sound::configuration& rynx::sound::configuration::set_attenuation_quadratic(float v) {
    if (is_active()) {
        m_soundData->m_attenuation.quadratic = v;
    }
    return *this;
}

rynx::sound::configuration& rynx::sound::configuration::set_attenuation_linear(float v) {
    if (is_active()) {
        m_soundData->m_attenuation.linear = v;
    }
    return *this;
}


float rynx::sound::configuration::completion_rate() const {
    if (!is_active())
        return 1.0f;
    uint32_t sample = m_soundData->m_sampleIndex;
    uint32_t buffer = m_soundData->m_bufferIndex;
    if (is_active()) {
        return float(sample) / float(m_rynxAudio->num_samples_in_buffer(buffer) + 1);
    }
    return 1;
}

bool rynx::sound::configuration::is_active() const {
    if (!m_soundData || !m_rynxAudio)
        return false;
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

rynx::sound::audio_system& rynx::sound::audio_system::load(std::string path, std::string event_name) {
    m_namedEvents.insert(event_name, load(path));
    return *this;
}


rynx::sound::configuration rynx::sound::audio_system::play_sound(int bufferId, vec3f position, vec3f direction, float loudness) {
    rynx_assert(bufferId <= m_soundBank.size() && bufferId >= 0, "out of bounds sound index!");
    rynx_assert(loudness >= 0.0f && loudness <= 10.0f, "really loud sound requested. is this intentional?");
    for (size_t i = 0; i < m_channels.size(); ++i) {
        uint32_t expected = 0;
        if (m_channels[i]->compare_exchange_weak(expected, 1u)) {
            m_channelDatas[i].m_bufferIndex = bufferId;
            m_channelDatas[i].m_sampleIndex = 0;
            m_channelDatas[i].m_position = position;
            m_channelDatas[i].m_direction = direction;
            m_channelDatas[i].m_loudness = loudness;
            m_channelDatas[i].m_attenuation.linear = m_default_linear_attenuation;
            m_channelDatas[i].m_attenuation.quadratic = m_default_quadratic_attenuation;
            m_channels[i]->store(2);
            return rynx::sound::configuration(*this, m_channelDatas[i]);
        }
    }

    rynx::sound::configuration result(*this, m_invalidChannel);
    ++m_invalidChannel.m_soundCounter; // invalidate config immediately.
    return result;
}

rynx::sound::configuration rynx::sound::audio_system::play_sound(const std::string& named_event, vec3f position, vec3f direction, float loudness) {
    return play_sound(m_namedEvents.get(named_event), position, direction, loudness);
}

void rynx::sound::audio_system::open_output_device(int numChannels, int samplesPerRender, rynx::sound::audio_system::format outputFormat) {

    setNumChannels(numChannels);

    m_outputFormat = outputFormat;

    {
        m_currentSampleRate = static_cast<float>(Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultSampleRate);
        std::cerr << "sample rate for default device: " << m_currentSampleRate << std::endl;

        int portAudioFormat = 0;
        if (m_outputFormat == format::int16)
            portAudioFormat = paInt16;
        if (m_outputFormat == format::int32)
            portAudioFormat = paInt32;
        if (m_outputFormat == format::float32)
            portAudioFormat = paFloat32;

        /* Open an audio I/O stream. */
        PaError err = Pa_OpenDefaultStream(&stream,
            0, // no input channels
            2, // stereo output
            portAudioFormat,
            m_currentSampleRate,
            samplesPerRender,
            audio_callback,
            this);

        if (err != paNoError) {
            std::cerr << "failed to open default audio output stream: " << std::string(Pa_GetErrorText(err)) << std::endl;
            m_outputFormat = format::undefined;
        }
    }

    {
        PaError err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "failed to start audio output stream: " << std::string(Pa_GetErrorText(err)) << std::endl;
            m_outputFormat = format::undefined;
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

void rynx::sound::audio_system::render_audio(void* deviceBuffer, size_t numSamples) {

    constexpr int numChannels = 2;
    if (m_outBufLength < numSamples * numChannels) {
        m_outBuf = std::unique_ptr<float[]>(new float[numSamples * numChannels]);
        m_outBufLength = numSamples * numChannels;
    }

    auto do_render = [this, numSamples](auto* deviceBuffer) {
        for (size_t i = 0; i < m_channels.size(); ++i) {
            if (m_channels[i]->load() == 2) {
                source_instance& data = m_channelDatas[i];

                rynx::vec3f soundOriginDirection = data.m_position - m_listenerPosition;
                float sqr_dist = soundOriginDirection.length_squared();
                float distance = data.m_attenuation.linear * rynx::math::sqrt_approx(sqr_dist) + data.m_attenuation.quadratic * sqr_dist;
                float volume = 1.0f / (distance + 1.0f);
                volume *= data.m_loudness;

                soundOriginDirection.normalize();
                float x = soundOriginDirection.dot(m_listenerLeft);
                float y = soundOriginDirection.dot(m_listenerUp);
                float z = soundOriginDirection.dot(m_listenerForward);
                
                // left/right volume balance
                float left_adj = y * y + z * z + x * x * x;
                float right_adj = y * y + z * z - x * x * x;
                left_adj = (left_adj + 1.0f) * 0.5f;
                right_adj = (right_adj + 1.0f) * 0.5f;

                // up/down frequency modifications
                // float high_freq_cull = std::max(y, 0.0f);
                // float low_freq_cull = std::max(-y, 0.0f);

                // front/back frequency modifications


                auto& sound = m_soundBank[data.m_bufferIndex];
                float resample_multiplier = sound.sampleRate / m_currentSampleRate;

                if constexpr (true) {
                    size_t fft_window = 2 * numSamples;
                    float multiplier = std::pow(2.0f, data.m_effects.pitch_shift);
                    float tempo_mul = std::pow(2.0f, data.m_effects.tempo_shift) * resample_multiplier;

                    multiplier *= tempo_mul;

                    std::valarray<std::complex<float>> chunk_left;
                    std::valarray<std::complex<float>> chunk_right;

                    chunk_left.resize(fft_window);
                    chunk_right.resize(fft_window);

                    int32_t num_samples_to_take = int32_t(int32_t(sound.left.size() - int32_t(data.m_sampleIndex) - 1) / multiplier);
                    if (num_samples_to_take > int32_t(fft_window))
                        num_samples_to_take = int32_t(fft_window);

                    for (size_t k = 0; k < num_samples_to_take; ++k) {
                        float src_index_f = multiplier * k;
                        size_t src_index_prev = size_t(src_index_f);
                        size_t src_index_next = size_t(src_index_f) + 1;
                        float p_prev = 1.0f - (src_index_f - src_index_prev);
                        float p_next = 1.0f - p_prev;

                        chunk_left[k].real(sound.left[src_index_prev + data.m_sampleIndex] * p_prev + sound.left[src_index_next + data.m_sampleIndex] * p_next);
                        chunk_right[k].real(sound.right[src_index_prev + data.m_sampleIndex] * p_prev + sound.right[src_index_next + data.m_sampleIndex] * p_next);
                    }

                    // up/down frequency mods
                    if constexpr (false) {
                        fft(chunk_left);
                        fft(chunk_right);

                        for (int32_t k = 0; k < num_samples_to_take; ++k) {
                            int32_t distance_from_spectral_center = std::abs(k - (num_samples_to_take >> 1));
                            float low = 2.0f * distance_from_spectral_center / num_samples_to_take;
                            float high = (num_samples_to_take - 2.0f * distance_from_spectral_center) / num_samples_to_take;

                            chunk_left[k].real(chunk_left[k].real() * (1.0f - low * low * low_freq_cull - high * high * high_freq_cull));
                            chunk_right[k].real(chunk_right[k].real() * (1.0f - low * low * low_freq_cull - high * high * high_freq_cull));
                        }

                        ifft(chunk_left);
                        ifft(chunk_right);
                    }

                    float numSamples_f = float(numSamples);
                    for (size_t k = 0; k < numSamples; ++k) {
                        float p = float(k) / numSamples_f;
                        m_outBuf[2 * k + 0] += left_adj * volume * (chunk_left[k].real() * p + data.prev_left[k + numSamples].real() * (1.0f - p));
                        m_outBuf[2 * k + 1] += right_adj * volume * (chunk_right[k].real() * p + data.prev_right[k + numSamples].real() * (1.0f - p));
                    }

                    data.prev_left = std::move(chunk_left);
                    data.prev_right = std::move(chunk_right);

                    data.m_sampleIndex += static_cast<uint32_t>(numSamples * tempo_mul);
                }
                else {
                    // no effects branch.
                    for (size_t k = 0; (static_cast<int>(k * resample_multiplier) + data.m_sampleIndex + 1 < sound.left.size()) & (k < numSamples); ++k) {
                        float base = k * resample_multiplier;
                        int base_i = static_cast<int>(base);
                        size_t index1 = static_cast<int>(k * resample_multiplier);
                        size_t index2 = index1 + 1;

                        float mul1 = 1.0f - (base - base_i);
                        float mul2 = 1.0f - mul1;

                        m_outBuf[2 * k + 0] += left_adj * volume * (sound.left[index1 + data.m_sampleIndex] * mul1 + sound.left[index2 + data.m_sampleIndex] * mul2);
                        m_outBuf[2 * k + 1] += right_adj * volume * (sound.right[index1 + data.m_sampleIndex] * mul1 + sound.right[index2 + data.m_sampleIndex] * mul2);
                    }
                    data.m_sampleIndex += static_cast<uint32_t>(numSamples * resample_multiplier);
                }

                // sound playback complete.
                if (data.m_sampleIndex >= sound.left.size()) {
                    data.m_sampleIndex = 0;
                    data.m_bufferIndex = 0;
                    data.m_loudness = 0;

                    data.m_effects.pitch_shift = 0;
                    data.m_effects.tempo_shift = 0;

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
        for (size_t i = 0; i < numSamples * numChannels; ++i) {
            float v = m_outBuf[i] * m_volumeScaleCurrent;
            v *= v;
            max = (v > max) ? v : max;
        }

        float scaleMultiplier = 1.04f - 0.08f * (max > 0.5f);
        m_volumeScaleCurrent *= scaleMultiplier;
        m_volumeScaleCurrent = std::min(m_volumeScaleCurrent, 1.0f);

        for (size_t i = 0; i < numSamples * numChannels; ++i) {
            float volume_i = m_outBuf[i] * m_volumeScaleCurrent;
            float mixed_float_value = std::tan(volume_i * m_globalVolume);

            using output_format_t = std::remove_pointer_t<decltype(deviceBuffer)>;
            if constexpr (std::is_same_v<output_format_t, int16_t>) {
                *deviceBuffer++ = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * mixed_float_value);
            }
            if constexpr (std::is_same_v<output_format_t, int32_t>) {
                *deviceBuffer++ = static_cast<int32_t>(std::numeric_limits<int32_t>::max() * mixed_float_value);
            }
            if constexpr (std::is_same_v<output_format_t, float>) {
                *deviceBuffer++ = mixed_float_value;
            }
        }
    };

    std::memset(m_outBuf.get(), 0, sizeof(float) * m_outBufLength);
    if (m_outputFormat == audio_system::format::int16) {
        std::memset(deviceBuffer, 0, sizeof(int16_t) * numSamples * numChannels);
        do_render(static_cast<int16_t*>(deviceBuffer));
    }
    else if(m_outputFormat == audio_system::format::int32)
    {
        std::memset(deviceBuffer, 0, sizeof(int32_t) * numSamples * numChannels);
        do_render(static_cast<int32_t*>(deviceBuffer));
    }
    else if(m_outputFormat == audio_system::format::float32)
    {
        std::memset(deviceBuffer, 0, sizeof(float) * numSamples * numChannels);
        do_render(static_cast<float*>(deviceBuffer));
    }
}

