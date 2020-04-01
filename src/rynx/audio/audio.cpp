
#include <portaudio.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#include <rynx/tech/math/vector.hpp>
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

static int audio_callback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    static_cast<rynx::sound::audio_system*>(userData)->render_audio(static_cast<float*>(outputBuffer), framesPerBuffer);
    return 0;
}

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
        return (of->curPtr - of->filePtr);
    }
}

rynx::sound::buffer loadOggVorbis(std::string path) {
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

    int ret = ov_open_callbacks((void*)&t, &ov, NULL, -1, callbacks);

    vorbis_info* vi = ov_info(&ov, -1);
    rynx_assert(vi);

    int num_channels = vi->channels;
    int sample_rate = vi->rate;
    int64_t total_samples = ov_pcm_total(&ov, -1);
    
    rynx::sound::buffer buffer;
    buffer.left.resize(total_samples);
    buffer.right.resize(total_samples);

    long samples_read = 0;
    int current_section = 0;
    while (true) {
        float** pcm;
        long ret = ov_read_float(&ov, &pcm, total_samples, &current_section);
        if (ret == 0) {
            logmsg("loaded ogg with %d samples: '%s'", samples_read, path.c_str());
            break;
        }
        else if (ret < 0) {
            logmsg("failed to load: '%s'", path.c_str());
        }
        else {
            // successfully got some data. copy it to our buffer.
            for (long i = 0; i < ret; ++i) {
                buffer.left[samples_read + i] = pcm[0][i];
                buffer.right[samples_read + i] = pcm[1][i];
            }
            samples_read += ret;
        }
    }

    ov_clear(&ov);
    return buffer;
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
    rynx_assert(m_soundData != nullptr);
    rynx_assert(m_rynxAudio != nullptr);
    return m_soundCounter == m_soundData->m_soundCounter;
}





rynx::sound::audio_system::audio_system() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        // oh no.
    }

    m_soundBank.emplace_back(); // guarantee that sound index zero points to a silent (and empty) sample.
}

rynx::sound::audio_system::~audio_system() {
    PaError err = Pa_CloseStream(stream);
    if (err != paNoError) {
        // oh no!
    }

    err = Pa_Terminate();
    if (err != paNoError) {
        // oh no.
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
    uint32_t soundIndex = m_soundBank.size();
    m_soundBank.emplace_back(loadOggVorbis(path));
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
        /* Open an audio I/O stream. */
        PaError err = Pa_OpenDefaultStream(&stream,
            0, // no input channels
            2, // stereo output
            paFloat32,
            44100,
            samplesPerRender,
            audio_callback,
            this);

        if (err != paNoError) {
            // oh no!
        }
    }

    {
        PaError err = Pa_StartStream(stream);
        if (err != paNoError) {
            // oh no!
        }
    }
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
            
            if (data.m_effects.pitch_shift != 0.0f) {
                Chunk chunk_left;
                Chunk chunk_right;

                chunk_left().resize(numSamples);
                chunk_right().resize(numSamples);

                if (data.m_sampleIndex + numSamples < sound.left.size()) {
                    for (size_t k = 0; k < numSamples; ++k) {
                        chunk_left()[k] = std::complex<float>(sound.left[k + data.m_sampleIndex], 0);
                        chunk_right()[k] = std::complex<float>(sound.right[k + data.m_sampleIndex], 0);
                    }
                }
                else {
                    for (size_t k = 0, end_index = sound.left.size(); k + data.m_sampleIndex < end_index; ++k) {
                        chunk_left()[k] = std::complex<float>(sound.left[k + data.m_sampleIndex], 0);
                        chunk_right()[k] = std::complex<float>(sound.right[k + data.m_sampleIndex], 0);
                    }
                    for (size_t k = sound.left.size(); k < numSamples; ++k) {
                        chunk_left()[k] = std::complex<float>(0, 0);
                        chunk_right()[k] = std::complex<float>(0, 0);
                    }
                }

                fft(chunk_left());
                fft(chunk_right());

                float multiplier = std::pow(2.0f, data.m_effects.pitch_shift);
                
                chunk_left.shiftGeometric(multiplier);
                chunk_right.shiftGeometric(multiplier);

                ifft(chunk_left());
                ifft(chunk_right());

                for (size_t k = 0; k < numSamples; ++k) {
                    outBuf[2 * k + 0] += volume * chunk_left()[k].real();
                    outBuf[2 * k + 1] += volume * chunk_right()[k].real();
                }
            }
            
            for (size_t k = 0; (k + data.m_sampleIndex < sound.left.size()) & (k < numSamples); ++k) {
                outBuf[2 * k + 0] += volume * sound.left[k + data.m_sampleIndex];
                outBuf[2 * k + 1] += volume * sound.right[k + data.m_sampleIndex];
            }

            data.m_sampleIndex += numSamples;

            // sound playback complete.
            if (data.m_sampleIndex >= sound.left.size()) {
                data.m_sampleIndex = 0;
                data.m_bufferIndex = 0;
                data.m_loudness = 0;
                ++data.m_soundCounter;
                m_channels[i]->store(0);
            }
        }
    }

    for (size_t i = 0; i < numSamples * 2; ++i) {
        outBuf[i] = std::tan(outBuf[i]); // lol mix hehe. quality.
    }
}