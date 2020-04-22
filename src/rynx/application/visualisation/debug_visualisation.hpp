#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <rynx/application/render.hpp>

#include <memory>
#include <vector>


namespace rynx {
	class mesh;
	class MeshRenderer;
	namespace scheduler {
		class context;
	}

	namespace application {
		class DebugVisualization : public igraphics_step {
		public:
			DebugVisualization(std::shared_ptr<MeshRenderer> meshRenderer) : m_meshRenderer(meshRenderer) {}

			void addDebugVisual(mesh* mesh, matrix4 m, floats4 color, float lifetime = 0.0f);
			
			virtual void prepare(rynx::scheduler::context* ctx) override;
			virtual void execute() override;

		private:
			struct buffer_obj {
				std::vector<rynx::matrix4> matrices;
				std::vector<floats4> colors;
				std::vector<float> lifetimes;
			};

			rynx::unordered_map<mesh*, buffer_obj> m_data;
			std::shared_ptr<MeshRenderer> m_meshRenderer;
		};
	}
}