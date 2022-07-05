
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/graphics/color.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/texture/id.hpp>

namespace rynx {
	namespace graphics {
		class mesh;
	}

	namespace graphics {
		class GPUTextures;
	}

	namespace menu {
		class Frame : public Component {
			vec3<float> m_prevScale;
			matrix4 m_modelMatrix;
			rynx::unique_ptr<rynx::graphics::mesh> m_backgroundMesh;
			rynx::graphics::texture_id m_backgroundMeshTexture;
			float m_edgeSize;

			void initMesh(floats4 uv_limits);

		public:
			Frame(
				rynx::graphics::texture_id id,
				float textureEdgeSize = 0.20f,
				float meshEdgeSize = 0.5f);
			
			void buildMesh(float size_x, float size_y);
			Frame& edge_size(float v) { m_edgeSize = v; return *this; }
			void set_texture(rynx::graphics::texture_id id) { m_backgroundMeshTexture = id; }

			virtual void update(float) override;

			virtual void onInput(rynx::mapped_input&) override {}
			virtual void draw(rynx::graphics::renderer& renderer) const override;
		};
	}
}
