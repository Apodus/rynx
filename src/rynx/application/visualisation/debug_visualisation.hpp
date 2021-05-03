#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <rynx/application/render.hpp>
#include <rynx/graphics/mesh/id.hpp>

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
			DebugVisualization(std::shared_ptr<rynx::graphics::renderer> meshRenderer);

			void addDebugLine(rynx::vec3f point_a, rynx::vec3f point_b, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugCircle(rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(rynx::graphics::mesh* mesh, matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(rynx::graphics::mesh_id mesh_id, rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(std::string meshName, rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);

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

			rynx::graphics::mesh_id m_circle_mesh;
			rynx::graphics::mesh_id m_line_mesh;
		};
	}
}