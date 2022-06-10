#pragma once

#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs/id.hpp>

namespace rynx {
	namespace ruleset {
		class physics_2d : public application::logic::iruleset {
		public:
			physics_2d() = default;
			virtual ~physics_2d() {}
			virtual void clear(rynx::scheduler::context&) override;
			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override;
			virtual void on_entities_erased(rynx::scheduler::context& context, const std::vector<rynx::id>& ids) override;
		};
	}
}
