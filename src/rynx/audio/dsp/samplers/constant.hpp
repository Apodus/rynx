#pragma once

#include <rynx/audio/dsp/sampler.hpp>

class Constant : public Sampler<Constant> {
public:
	Constant(float v = 0) {
		value = v;
	}

	virtual ~Constant() {}

	float val() const { return value; }
	Constant val(float v) { value = v; }

private:
	float baseSample(float time, float frequencyModifier = 1.0f) const override {
		return value;
	}

	float value;
};
