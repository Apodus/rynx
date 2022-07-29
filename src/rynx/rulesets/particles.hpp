
#pragma once

#include <rynx/application/logic.hpp>

namespace rynx {
	namespace ruleset {
		class RuleSetsDLL particle_system : public application::logic::iruleset {
		public:
			virtual ~particle_system();
			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
		};
	}
}