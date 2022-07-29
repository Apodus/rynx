#pragma once

#include <rynx/application/logic.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	namespace ruleset {
		class RuleSetsDLL motion_updates : public application::logic::iruleset {
			vec3<float> m_gravity;
		public:
			motion_updates(vec3<float> gravity = vec3<float>(0, 0, 0)) : m_gravity(gravity) {}
			virtual ~motion_updates() {}
			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
		};
	}
}
