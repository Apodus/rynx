
#pragma once


#include <rynx/application/logic.hpp>

namespace rynx {
	namespace ruleset {
		class lifetime_updates : public application::logic::iruleset {
		public:
			lifetime_updates() = default;
			virtual ~lifetime_updates() = default;

			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
		};
	}
}