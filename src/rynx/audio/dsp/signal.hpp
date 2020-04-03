#pragma once

#include <rynx/audio/dsp/fft.hpp>

#include <vector>
#include <iostream>
#include <valarray>
#include <cassert>

static void loadingPrint(size_t step, size_t max, size_t howMany = 10) {
	if (howMany * (step) / max != howMany * (step - 1) / max)
		std::cerr << "." << std::flush;
}

class Chunk {
private:
	void shiftLeft(size_t from, size_t to) {
		for (size_t i = from + 1; i < to; ++i) {
			signal[i - 1].real(signal[i].real());
			signal[i - 1].imag(signal[i].imag());
		}
		signal[to - 1].real(0);
		signal[to - 1].imag(0);
	}

	void shiftRight(size_t from, size_t to) {
		for (size_t i = to - 1; i > from; --i) {
			signal[i].real(signal[i - 1].real());
			signal[i].imag(signal[i - 1].imag());
		}
		signal[from].real(0);
		signal[from].imag(0);
	}

public:
	float amplitude() const {
		float sum = 0;
		for (const auto& f : signal) {
			sum += std::fabs(f.real());
		}
		return sum;
	}

	size_t maxBin() const {
		float maxAmplitude = 0;
		size_t maxIndex = 0;
		for (size_t i = 1; i < signal.size() / 2; ++i) {
			float frequencyAmplitude = std::abs(signal[i].real());
			if (frequencyAmplitude > maxAmplitude) {
				maxAmplitude = frequencyAmplitude;
				maxIndex = i;
			}
		}
		return maxIndex;
	}

	float maxVal() const {
		return signal[maxBin()].real();
	}

	Chunk& shiftLeft() {
		shiftLeft(0, signal.size() / 2);
		shiftRight(signal.size() / 2, signal.size());
		return *this;
	}

	Chunk& shiftRight() {
		shiftRight(0, signal.size() / 2);
		shiftLeft(signal.size() / 2, signal.size());
		return *this;
	}

	Chunk& shiftGeometric(float multiplier) {
		auto mid = signal.size() / 2;
		auto invert = [this](size_t i) { return signal.size() - 1 - i; };

		std::valarray<std::complex<float>> copy(std::complex<float>(0, 0), signal.size());

		{
			if (multiplier > 1.0f)
			{
				float inv_mul = 1.0f / multiplier;
				// this index is influenced by X.
				for (size_t i = 0; i < mid; ++i) {
					float source = float(i) * inv_mul;
					size_t source_min = size_t(source);
					size_t source_max = size_t(source) + 1;
					float p_min = 1.0f - (source - int(source));
					float p_max = 1.0f - p_min;

					copy[i] += signal[source_max] * p_max * 2.0f;
					// copy[invert(i)] += signal[invert(source_max)] * p_max;
				
					copy[i] += signal[source_min] * p_min * 2.0f;
					// copy[invert(i)] += signal[invert(source_min)] * p_min;
				}
			}
			else
			{
				// this index influences index X.
				for (size_t i = 0; i < mid; ++i) {
					float dst = float(i) * multiplier;
					size_t dst_min = size_t(dst);
					size_t dst_max = size_t(dst) + 1;
					float p_min = 1.0f - (dst - int(dst));
					float p_max = 1.0f - p_min;

					copy[dst_max] += signal[i] * p_max * 2.0f;
					// copy[invert(dst_max)] += signal[invert(i)] * p_max;
				
					copy[dst_min] += signal[i] * p_min * 2.0f;
					// copy[invert(dst_min)] += signal[invert(i)] * p_min;
				}
			}
		}

		signal = std::move(copy);
		return *this;
	}

	// doesn't matter which direction shifting, because of symmetry
	Chunk& scatter() {
		shiftLeft(0, signal.size() / 2);
		shiftLeft(signal.size() / 2, signal.size());
		return *this;
	}

	Chunk& filterLows(int preserveCount = 1) {
		std::vector<std::pair<size_t, std::complex<float>>> stash;
		for (int i = 0; i < preserveCount; ++i) {
			size_t index = maxBin();
			stash.emplace_back(index, signal[index]);
			signal[index].real(0);
			signal[index].imag(0);
		}

		for (size_t i = 0; i < signal.size(); ++i) {
			signal[i].real(0);
			signal[i].imag(0);
		}

		for (int i = 0; i < preserveCount; ++i) {
			signal[stash[i].first] = stash[i].second;
		}

		return *this;
	}

	std::valarray<std::complex<float>>& operator ()() {
		return signal;
	}

	const std::valarray<std::complex<float>>& operator ()() const {
		return signal;
	}

	float frequency(size_t samplingRate = 44100) const {
		return binFrequency(maxBin(), samplingRate);
	}

	float binFrequency(size_t binIndex, size_t samplingRate = 44100) const {
		return static_cast<float>(binIndex * samplingRate) / signal.size();
	}

	void visualize(size_t samplingRate = 44100) const {
		float max = std::fabs(maxVal());
		for (size_t i = 0; i < signal.size() / 2; ++i) {
			std::cout << binFrequency(i, samplingRate) << " Hz: ";
			float amp = std::fabs(signal[i].real());
			int visualAmp = static_cast<int>(70 * (amp / max));

			for (int k = 0; k < visualAmp; ++k) {
				std::cout << "#";
			}
			
			std::cout << std::endl;
		}
	}

private:
	std::valarray<std::complex<float>> signal;
};

template<typename F>
inline void chunkify1D(std::vector<int> source, size_t window, size_t jump, F&& func) {
	for (size_t startPoint = 0; startPoint + window <= source.size(); startPoint += jump) {
		loadingPrint(startPoint / jump, source.size() / jump);
		Chunk chunk;
		chunk().resize(window);
		for (size_t i = 0; i < window; ++i) {
			chunk()[i].real(float(source[startPoint + i]));
		}
		func(std::move(chunk));
	}
}

inline void renderChunk(std::vector<int>& output, const Chunk& chunk, size_t jump, size_t i, float amplitudeMultiplier = 1.0f) {
	auto chirp = [](float v, size_t i, size_t windowSize, size_t jumpSize) {
		size_t distanceBegin = i;
		size_t distanceEnd = windowSize - i;
		size_t distance = distanceBegin < distanceEnd ? distanceBegin : distanceEnd;
		float overlapMultiplier = float(jumpSize) / float(windowSize);
		float chirp = overlapMultiplier * 2.0f * float(distance) / float(windowSize);
		return v * chirp;
	};

	size_t currentSpot = i * jump;
	
	auto copy = chunk();
	rynx::ifft(copy);

	assert(output.size() >= currentSpot + copy.size());
	for (size_t step = 0; step < copy.size(); ++step) {
		output.data()[currentSpot + step] += int(amplitudeMultiplier * chirp(copy[step].real(), step, copy.size(), jump));
	}
}

inline void renderChunkIFFT(std::vector<int>& output, const Chunk& chunk, size_t jump, size_t i) {
	auto chirp = [](float v, size_t i, size_t windowSize, size_t jumpSize) {
		size_t distanceBegin = i;
		size_t distanceEnd = windowSize - i;
		size_t distance = distanceBegin < distanceEnd ? distanceBegin : distanceEnd;
		float overlapMultiplier = float(jumpSize) / float(windowSize);
		float chirp = overlapMultiplier * 2.0f * float(distance) / float(windowSize);
		return v * chirp * 0.98f;
	};

	size_t currentSpot = i * jump;
	auto copy = chunk();
	rynx::ifft(copy);

	assert(output.size() >= currentSpot + copy.size());
	for (size_t step = 0; step < copy.size(); ++step) {
		output.data()[currentSpot + step] += int(chirp(copy[step].real(), step, copy.size(), jump));
	}
}

class NFrequencySignal {
public:
	NFrequencySignal& fromTimeDomain(std::vector<int> source, size_t window, size_t jump) {
		std::cerr << __FUNCTION__ << std::flush;
		windowSize = window;
		size_t sourceSize = source.size();
		chunkify1D(std::move(source), window, jump, [this](Chunk chunk) {
			rynx::fft(chunk());
			uint32_t index = static_cast<uint32_t>(chunk.maxBin());
			float amplitude = chunk()[index].real();
			chunks.emplace_back(index, amplitude);
		});
		std::cerr << "done (" << sourceSize << " samples, " << chunks.size() << " blocks)" << std::endl;
		return *this;
	}

	std::vector<int> render(size_t jump) {
		if (chunks.empty())
			return {};
		std::cerr << __FUNCTION__ << std::flush;

		std::vector<int> output;
		output.resize(windowSize + jump * (chunks.size() - 1));

		for (size_t i = 0; i < chunks.size(); ++i) {
			loadingPrint(i, chunks.size());
			Chunk chunk;
			chunk().resize(windowSize, std::complex<float>(0, 0));
			chunk()[chunks[i].first].real(chunks[i].second);
			// std::cerr << chunks[i].first << " " << chunks[i].second << std::endl;
			renderChunk(output, chunk, jump, i);
		}
		std::cerr << "done (" << output.size() << " samples)" << std::endl;
		return output;
	}

	template<typename T>
	NFrequencySignal& apply(T&& t) {
		for (auto&& chunk : chunks)
			t(chunk);
		return *this;
	}

	std::vector<std::pair<uint32_t, float>> chunks;
	size_t windowSize = 0;
};

class Signal {
public:
	using Chunk = ::Chunk;
	Signal& fromTimeDomain(std::vector<int> source, size_t window, size_t jump) {
		size_t sourceSize = source.size();
		std::cerr << __FUNCTION__ << std::flush;
		chunkify1D(std::move(source), window, jump, [this](Chunk chunk) {
			rynx::fft(chunk());
			chunks.emplace_back(std::move(chunk));
		});
		std::cerr << "done (" << sourceSize << " samples, " << chunks.size() << " blocks)" << std::endl;
		return *this;
	}

	std::vector<int> render(size_t jump) {
		if (chunks.empty())
			return {};
		std::cerr << __FUNCTION__ << std::flush;

		std::vector<int> output;
		output.resize(chunks[0]().size() + jump * (chunks.size() - 1));
		
		for (size_t i = 0; i < chunks.size(); ++i) {
			loadingPrint(i, chunks.size());
			renderChunk(output, chunks[i], jump, i);
		}
		std::cerr << "done (" << output.size() << " samples)" << std::endl;
		return output;
	}

	template<typename T>
	Signal& apply(T&& t) {
		for (auto&& chunk : chunks)
			t(chunk);
		return *this;
	}

	size_t size() const {
		return chunks.size();
	}

	const Chunk& chunk(size_t index) const {
		return chunks[index];
	}

private:
	std::vector<Chunk> chunks;
};

