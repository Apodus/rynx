#pragma once

#include <rynx/application/logic.hpp>

namespace rynx {
	namespace ruleset {
		namespace physics {
			class RuleSetsDLL springs : public application::logic::iruleset {
			public:
				springs() = default;
				virtual ~springs() {}
				virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
			};
		}
	}
}
