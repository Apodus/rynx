#pragma once

#include <vector>
#include <cstdint>
#include <istream>

namespace wave {
	struct Wave {
	public:
		Wave() {}
		Wave(std::vector<int>&& samples, int sampleRate, int channels) : m_samples(std::move(samples)), m_sampleRate(sampleRate), m_channels(channels) {}

		std::vector<int>&& samples() { return std::move(m_samples); }
		int sampleRate() { return m_sampleRate; }
		Wave& sampleRate(int dstSampleRate) {
			if (m_samples.empty())
				return *this;
			if (dstSampleRate == m_sampleRate)
				return *this;

			float dst = float(dstSampleRate);
			float src = float(m_sampleRate);
			float relative = src / dst;
			size_t i = 0;

			if (dstSampleRate < m_sampleRate) {
				// write beginning first, do in-place
				while(true) {
					float fromIndex = relative * i;
					size_t a = size_t(fromIndex);
					size_t b = a + 1;

					if (b >= m_samples.size())
					{
						// we done here mkay.
						m_samples.resize(i);
						break;
					}

					float p_b = fromIndex - a;
					float p_a = 1 - p_b;

					m_samples[i] = static_cast<int>(p_a * m_samples[a] + p_b * m_samples[b]);
					++i;
				}
			}
			else {
				// take a copy since otherwise we end up overwriting the src data before we are reading it.
				std::vector<int> samplesCopy = m_samples;
				m_samples.resize(static_cast<size_t>(relative * m_samples.size()+1), 0);
				for (; i < m_samples.size(); ++i) {
					float fromIndex = relative * i;
					size_t a = size_t(fromIndex);
					size_t b = a + 1;

					if (b >= samplesCopy.size())
					{
						// we done here mkay.
						break;
					}

					float p_b = fromIndex - a;
					float p_a = 1 - p_b;

					m_samples[i] = static_cast<int>(p_a * samplesCopy[a] + p_b * samplesCopy[b]);
				}
			}

			return *this;
		}

		std::vector<int> m_samples;
		int m_sampleRate = 0;
		int m_channels = 0;
	};

	struct wav_hdr {
		/* RIFF Chunk Descriptor */
		uint8_t RIFF[4];        // RIFF Header Magic header
		uint32_t ChunkSize;      // RIFF Chunk Size
		uint8_t WAVE[4];        // WAVE Header
														/* "fmt" sub-chunk */
		uint8_t fmt[4]; // FMT header
		uint32_t Subchunk1Size;  // Size of the fmt chunk
		uint16_t AudioFormat; // Audio format 1=PCM,6=mulaw,7=alaw,257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
		uint16_t NumOfChan;
		uint32_t SamplesPerSec;
		uint32_t bytesPerSec;
		uint16_t blockAlign;     // 2=16-bit mono, 4=16-bit stereo
		uint16_t bitsPerSample;

		uint8_t Subchunk2ID[4]; // "data"  string
		uint32_t Subchunk2Size;

		template<typename Readable>
		void read(Readable&& in) {
			in.read(reinterpret_cast<char*>(RIFF), 4);
			in.read(reinterpret_cast<char*>(&ChunkSize), 4);
			in.read(reinterpret_cast<char*>(WAVE), 4);
			in.read(reinterpret_cast<char*>(fmt), 4);
			in.read(reinterpret_cast<char*>(&Subchunk1Size), 4);
			in.read(reinterpret_cast<char*>(&AudioFormat), 2);
			in.read(reinterpret_cast<char*>(&NumOfChan), 2);
			in.read(reinterpret_cast<char*>(&SamplesPerSec), 4);
			in.read(reinterpret_cast<char*>(&bytesPerSec), 4);
			in.read(reinterpret_cast<char*>(&blockAlign), 2);
			in.read(reinterpret_cast<char*>(&bitsPerSample), 2);
			in.ignore(Subchunk1Size - 16);
			in.read(reinterpret_cast<char*>(Subchunk2ID), 4);
			in.read(reinterpret_cast<char*>(&Subchunk2Size), 4);

			while (true) {
				if (memcmp(Subchunk2ID, "data", 4) == 0 || memcmp(Subchunk2ID, "DATA", 4) == 0) {
					// ok we done.
					break;
				}
				else {
					in.ignore(Subchunk2Size);
					in.read(reinterpret_cast<char*>(Subchunk2ID), 4);
					in.read(reinterpret_cast<char*>(&Subchunk2Size), 4);
				}
			}

#if 0
			std::cerr << "RIFF: " << RIFF << std::endl;
			std::cerr << "ChunkSize: " << ChunkSize << std::endl;
			std::cerr << "WAVE: " << WAVE << std::endl;
			std::cerr << "fmt: " << fmt << std::endl;
			std::cerr << "SubChunk1: " << Subchunk1Size << std::endl;
			std::cerr << "AudioFormat: " << AudioFormat << std::endl;
			std::cerr << "NumOfChannels: " << NumOfChan << std::endl;
			std::cerr << "SamplesPerSec: " << SamplesPerSec << std::endl;
			std::cerr << "bytesPerSec: " << bytesPerSec << std::endl;
			std::cerr << "blockAlign: " << blockAlign << std::endl;
			std::cerr << "bitsPerSample: " << bitsPerSample << std::endl;
			std::cerr << "SubChunk2ID: " << Subchunk2ID << std::endl;
			std::cerr << "SubChunk2Size: " << Subchunk2Size << std::endl;
#endif
		}
	};

	// assume 16bit samples.
	std::vector<uint8_t> saveMonoSignalAsWavPCM(const std::vector<int>& signal, int samplesPerSec) {
		constexpr int bytesPerSample = 2;
		std::vector<uint8_t> sample;

		wav_hdr header;
		header.ChunkSize = static_cast<uint32_t>(sizeof(wav_hdr) + signal.size() * bytesPerSample - 8);
		memcpy(header.RIFF, "RIFF", 4);
		memcpy(header.WAVE, "WAVE", 4);
		memcpy(header.fmt, "fmt ", 4);
		memcpy(header.Subchunk2ID, "data", 4);

		header.Subchunk1Size = 16;
		header.AudioFormat = 1;
		header.NumOfChan = 1;
		header.SamplesPerSec = samplesPerSec;
		header.bytesPerSec = samplesPerSec * bytesPerSample;
		header.blockAlign = 2;
		header.bitsPerSample = bytesPerSample * 8;
		header.Subchunk2Size = static_cast<uint32_t>(signal.size() * bytesPerSample);

		std::vector<int16_t> signal16;
		signal16.reserve(signal.size());

		for (int v : signal)
			signal16.emplace_back(int16_t(v));

		sample.resize(sizeof(wav_hdr) + signal16.size() * 2);
		memcpy(&sample[0], &header, sizeof(wav_hdr));
		memcpy(&sample[sizeof(wav_hdr)], &signal16[0], signal16.size() * 2);
		return sample;
	}

	// todo: should convert all signals to 16bit sample size
	template<typename Readable>
	Wave loadWavPCMAsMonoSignal(Readable&& wavFile) {
		wav_hdr wavHeader;
		int headerSize = sizeof(wav_hdr), filelength = 0;

		wavHeader.read(wavFile);

		//Read the data
		uint16_t bytesPerSample = wavHeader.bitsPerSample / 8;
		uint32_t numSamples = wavHeader.Subchunk2Size / bytesPerSample / wavHeader.NumOfChan;
		std::cerr << "header says " << numSamples << std::endl;
		std::vector<char> readBuffer;
		readBuffer.resize(bytesPerSample * numSamples * wavHeader.NumOfChan);
		wavFile.read(reinterpret_cast<char*>(readBuffer.data()), wavHeader.Subchunk2Size);

		std::vector<int32_t> samples;
		samples.resize(numSamples);

		if (wavHeader.AudioFormat != 1)
			return {};

		if (bytesPerSample == 1) {
			for (size_t i = 0; i < samples.size(); ++i) {
				samples[i] = (static_cast<int>(readBuffer[i * wavHeader.NumOfChan]) - 128) * 250;
			}
		}
		else if (bytesPerSample == 2) {
			for (size_t i = 0; i < samples.size(); ++i) {
				int16_t v;
				memcpy(&v, &readBuffer[i * 2 * wavHeader.NumOfChan], 2);
				samples[i] = v;
			}
		}
		else if (bytesPerSample == 3) {
			return {}; // ignore 3byte samples. too hard.
		}
		else if (bytesPerSample == 4) {
			for (size_t i = 0; i < samples.size(); ++i) {
				float v;
				memcpy(&v, &readBuffer[i * 4 * wavHeader.NumOfChan], 4);
				samples[i] = static_cast<int32_t>(v * ((1 << 15) - 100));
			}
		}

		return Wave(std::move(samples), wavHeader.SamplesPerSec, wavHeader.NumOfChan);
	}
}