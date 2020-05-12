
#pragma once

#include <rynx/application/logic.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <memory>

namespace rynx {
	class ecs;
	class camera;

	namespace ruleset {
		class frustum_culling : public application::logic::iruleset {
			struct entity_tracked_by_frustum_culling {};
		
		public:
			frustum_culling(std::shared_ptr<camera> camera) 
				: m_pCamera(std::move(camera))
			{}
			
			virtual ~frustum_culling() {}
			
			virtual void clear() override;
			virtual void on_entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids) override;
			virtual void onFrameProcess(rynx::scheduler::context& context, float /* dt */) override;

		private:
			rynx::sphere_tree m_in_frustum;
			rynx::sphere_tree m_out_frustum;
			std::shared_ptr<camera> m_pCamera;
		};
	}
}
