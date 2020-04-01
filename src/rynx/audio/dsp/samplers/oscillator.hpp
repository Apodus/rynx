#pragma once

#include <rynx/audio/dsp/sampler.hpp>

class Oscillator : public Sampler<Oscillator> {
	static constexpr float pi = 3.14159265359f;

public:
	Oscillator(float samplingRate = 44100.0f, float frequency = 440.0f, float volume = 1.0f)
		: freqMul(2 * pi * frequency / samplingRate)
		, amplitudeMul(volume)
		, samplingRate(samplingRate)
		, freq(frequency) {
	}

	virtual ~Oscillator() {}

	Oscillator(Oscillator&&) = default;
	Oscillator(const Oscillator&) = default;
	Oscillator& operator = (Oscillator&&) = default;
	Oscillator& operator = (const Oscillator&) = default;

	float amplitude() const { return amplitudeMul; }
	Oscillator& amplitude(float v) { amplitudeMul = v; return *this; }

	float frequency() const { return freq; }
	Oscillator& frequency(float f) { freq = f; freqMul = 2 * pi * freq / samplingRate; return *this; }

private:
	virtual float baseSample(float time, float frequencyModifier = 1.0f) const override {
		return amplitudeMul * std::sin(freqMul * time * frequencyModifier);
	}

private:
	float freqMul;
	float amplitudeMul;

	float samplingRate;
	float freq;
};
