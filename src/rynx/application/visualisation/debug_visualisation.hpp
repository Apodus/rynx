#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <rynx/application/render.hpp>

#include <memory>
#include <vector>


namespace rynx {
	namespace graphics {
		class mesh;
		class renderer;
	}
	namespace scheduler {
		class context;
	}

	namespace application {
		class DebugVisualization : public igraphics_step {
		public:
			DebugVisualization(std::shared_ptr<rynx::graphics::renderer> meshRenderer) : m_meshRenderer(meshRenderer) {}

			void addDebugVisual(rynx::graphics::mesh* mesh, matrix4 m, floats4 color, float lifetime = 0.0f);
			
			virtual void prepare(rynx::scheduler::context* ctx) override;
			virtual void execute() override;

		private:
			struct buffer_obj {
				std::vector<rynx::matrix4> matrices;
				std::vector<floats4> colors;
				std::vector<float> lifetimes;
				std::vector<rynx::graphics::texture_id> textures;
			};

			rynx::unordered_map<rynx::graphics::mesh*, buffer_obj> m_data;
			std::shared_ptr<rynx::graphics::renderer> m_meshRenderer;
		};
	}
}