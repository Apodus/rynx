#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/std/unordered_map.hpp>

#include <rynx/application/render.hpp>
#include <rynx/graphics/mesh/id.hpp>

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
		class ApplicationDLL DebugVisualization : public igraphics_step {
		public:
			DebugVisualization(rynx::shared_ptr<rynx::graphics::renderer> meshRenderer);

			void addDebugLine(rynx::vec3f point_a, rynx::vec3f point_b, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugLine_world(rynx::vec3f point_a, rynx::vec3f point_b, rynx::floats4 color, float width, float lifetime = 0.0f);
			void addDebugCircle(rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(rynx::graphics::mesh* mesh, matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(rynx::graphics::mesh_id mesh_id, rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);
			void addDebugVisual(rynx::string meshName, rynx::matrix4 model, rynx::floats4 color, float lifetime = 0.0f);

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
			rynx::shared_ptr<rynx::graphics::renderer> m_meshRenderer;

			rynx::graphics::mesh_id m_circle_mesh;
			rynx::graphics::mesh_id m_line_mesh;
			rynx::graphics::mesh_id m_box_mesh;
		};
	}
}