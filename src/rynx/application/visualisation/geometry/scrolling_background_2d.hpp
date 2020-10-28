
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	class camera;
	class mesh;
	class MeshRenderer;

	namespace application {
		namespace visualization {
			class scrolling_background_2d : public rynx::application::igraphics_step {
			public:
				scrolling_background_2d(rynx::MeshRenderer& mesh_renderer, std::shared_ptr<rynx::camera> camera, rynx::mesh* m);
				virtual ~scrolling_background_2d() {}
				virtual void execute() override;
				virtual void prepare(rynx::scheduler::context* ctx);
				void set_aspect_ratio(float aspect_ratio);
				void set_motion_scale(float x, float y) { horizontal_motion_scale = x; vertical_motion_scale = y; }

			private:
				rynx::mesh* m_bg_mesh = nullptr;
				std::shared_ptr<rynx::camera> m_camera;
				rynx::MeshRenderer& m_mesh_renderer;
				rynx::vec3f m_pos;
				float m_scale = 1.0f;
				float m_aspect_ratio = 1.0f;

				float horizontal_motion_scale = 1.0f;
				float vertical_motion_scale = 1.0f;
			};
		}
	}
}