
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/graphics/color.hpp>

#include <memory>

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
			std::unique_ptr<rynx::graphics::mesh> m_backgroundMesh;
			float m_edgeSize;

			void initMesh(floats4 uv_limits);

		public:
			Frame(
				rynx::graphics::GPUTextures& textures,
				const std::string& textureID,
				float edgeSize = 0.20f);
			
			void buildMesh(float size_x, float size_y);
			Frame& edge_size(float v) { m_edgeSize = v; return *this; }
			
			virtual void update(float) override;

			virtual void onInput(rynx::mapped_input&) override {}
			virtual void draw(rynx::graphics::renderer& meshRenderer, rynx::graphics::text_renderer&) const override;
		};
	}
}