#pragma once

#include <vector>
#include <memory>

class SamplerBase {
public:
	virtual ~SamplerBase() {}

	float sample(float time, float frequencyModifier = 1.0f) const {
		auto func = [](const auto& moduloSource, float time, float frequencyModifier) -> float {
			float mod = 1.0f;
			for (const auto& sampler : moduloSource) {
				mod *= sampler->sample(time, frequencyModifier);
			}
			return mod;
		};

		float amplitudeModulation = func(m_modulateAmplitude, time, frequencyModifier);
		float frequencyModulation = func(m_modulateFrequency, time, frequencyModifier);
		float phaseModulation = func(m_modulatePhase, time, frequencyModifier);
		float signalOffset = 0;
		for (const auto& sampler : m_modulateAmplitudeOffset) {
			signalOffset += sampler->sample(time, frequencyModifier);
		}

		return baseSample(time * frequencyModulation + phaseModulation, frequencyModifier) * amplitudeModulation + signalOffset;
	}

private:
	virtual float baseSample(float time, float frequencyModifier = 1.0f) const = 0;

protected:
	std::vector<std::shared_ptr<SamplerBase>> m_modulateFrequency;
	std::vector<std::shared_ptr<SamplerBase>> m_modulateAmplitude;
	std::vector<std::shared_ptr<SamplerBase>> m_modulatePhase;
	std::vector<std::shared_ptr<SamplerBase>> m_modulateAmplitudeOffset;
};

template<typename SamplerImpl>
class Sampler : public SamplerBase {
public:
	virtual ~Sampler() {}

	template<typename Samp>
	SamplerImpl& modulateAmp(Samp&& samp) {
		modulateAmp(std::shared_ptr<SamplerBase>(new std::decay_t<Samp>(std::move(samp))));
		return static_cast<SamplerImpl&>(*this);
	}

	template<typename Samp>
	SamplerImpl& modulateFreq(Samp&& samp) {
		modulateFreq(std::shared_ptr<SamplerBase>(new std::decay_t<Samp>(std::move(samp))));
		return static_cast<SamplerImpl&>(*this);
	}

	template<typename Samp>
	SamplerImpl& modulatePhase(Samp&& samp) {
		modulatePhase(std::shared_ptr<SamplerBase>(new std::decay_t<Samp>(std::move(samp))));
		return static_cast<SamplerImpl&>(*this);
	}

	template<typename Samp>
	SamplerImpl& volumeOffset(Samp&& samp) {
		volumeOffset(std::shared_ptr<SamplerBase>(new std::decay_t<Samp>(std::move(samp))));
		return static_cast<SamplerImpl&>(*this);
	}

	SamplerImpl& modulateAmp(std::shared_ptr<SamplerBase> sampler) {
		m_modulateAmplitude.emplace_back(std::move(sampler));
		return static_cast<SamplerImpl&>(*this);
	}

	SamplerImpl& modulateFreq(std::shared_ptr<SamplerBase> sampler) {
		m_modulateFrequency.emplace_back(std::move(sampler));
		return static_cast<SamplerImpl&>(*this);
	}

	SamplerImpl& modulatePhase(std::shared_ptr<SamplerBase> sampler) {
		m_modulatePhase.emplace_back(std::move(sampler));
		return static_cast<SamplerImpl&>(*this);
	}

	SamplerImpl& volumeOffset(std::shared_ptr<SamplerBase> sampler) {
		m_modulateAmplitudeOffset.emplace_back(std::move(sampler));
		return static_cast<SamplerImpl&>(*this);
	}
};
