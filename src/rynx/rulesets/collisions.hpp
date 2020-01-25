#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/math/vector.hpp>

namespace rynx {
	namespace ruleset {
		class physics_2d : public application::logic::iruleset {
			vec3<float> m_gravity;
		public:
			physics_2d(vec3<float> gravity = vec3<float>(0, 0, 0)) : m_gravity(gravity) {}
			virtual ~physics_2d() {}

			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
		};
	}
}
