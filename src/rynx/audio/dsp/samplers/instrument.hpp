#pragma once

#include <rynx/audio/dsp/samplers/oscillator.hpp>
#include <rynx/audio/dsp/signal.hpp>

#include <iostream>
#include <vector>

// TODO: should define decay sustain + decay release.
// TODO: should allow signal modifiers (modulators on amplitude, frequency, ?)
//       how to attach them?
class Instrument : public Sampler<Instrument> {
public:
	Instrument(std::string name, int samplingRate, float frequency, int attackTime_ms)
		: samplingRate(samplingRate), attackTime(attackTime_ms * samplingRate / 1000) {
		this->name = std::move(name);
		this->frequency = frequency;
	}

	virtual ~Instrument() {}

	void initialize() {
		std::vector<int> gen;
		for (int i = 0; i < (1 << 16); ++i) {
			gen.emplace_back(static_cast<int>(16000.0f * sample(float(i), 1.0f)));
		}

		float proposedBaseFrequency = static_cast<float>(Signal().fromTimeDomain(gen, 1 << 16, 1 << 15).chunk(0).frequency(samplingRate));
		baseFrequency = proposedBaseFrequency;
		std::cout << "loaded instrument '" << name << "' with a sample base frequency of " << int(baseFrequency) << "Hz" << std::endl;
	}

	float baseFreq() const {
		return baseFrequency;
	}

	Oscillator& harmonic(int t) {
		if (t - 1 >= static_cast<int>(oscillators.size())) {
			auto oldSize = static_cast<int>(oscillators.size());
			oscillators.resize(t);
			for (auto i = oldSize; i < t; ++i) {
				oscillators[i].frequency(frequency * i);
			}
		}
		return oscillators[t - 1];
	}

private:
	virtual float baseSample(float time, float frequencyModifier = 1.0f) const override {
		if (time < attackTime) {
			return (sustain(time * frequencyModifier) * time / attackTime);
		}
		return (sustain(time * frequencyModifier));
	}

	inline float sustain(float time) const {
		float v = 0;
		for (auto&& osc : oscillators) {
			v += osc.sample(time);
		}
		return v;
	}

	int attackTime;
	int samplingRate;

	float frequency; // not sure what this actually describes. like. in reality.
	float baseFrequency = 0;
	std::vector<Oscillator> oscillators;
	std::string name;
};
