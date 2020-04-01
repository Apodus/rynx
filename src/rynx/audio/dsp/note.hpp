#pragma once

#include "samplers/instrument.hpp"

// TODO: should define sustain time
class Note {
public:
	Note(Instrument& instrument, float freq, float pow, float time) : instrument(instrument), frequency(freq), power(pow), duration_ms(time) {}
	void renderOn(std::vector<int>& out, size_t pos, size_t sampleRate = 44100) {
		int sampleCount = static_cast<int>(duration_ms * sampleRate / 1000);
		float multiPlier = frequency / instrument.baseFreq();
		int fallOffCut = 90 * sampleCount / 100;

		for (int i = 0; i < fallOffCut; ++i) {
			float v = instrument.sample(float(i), multiPlier) * power;
			out[pos + i] += static_cast<int>(v);
		}

		for (int i = fallOffCut; i < sampleCount; ++i) {
			float v = instrument.sample(float(i), multiPlier) * power;
			int donePercent = 100 * (i - fallOffCut) / (sampleCount - fallOffCut);
			out[pos + i] += static_cast<int>(v * (100 - donePercent) / 100);
		}
	}

private:
	Instrument& instrument;
	float frequency;
	float power;
	float duration_ms;
};
