
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/graphics/color.hpp>

#include <memory>

class GPUTextures;

namespace rynx {
	class mesh;
	namespace menu {
		class Frame : public Component {

			floats4 m_color;
			vec3<float> m_prevScale;
			std::string m_textureID;
			matrix4 m_modelMatrix;
			std::unique_ptr<mesh> m_backgroundMesh;
			float m_edgeSize;

			void initMesh(GPUTextures& textures);

		public:
			Frame(
				Component* parent,
				GPUTextures& textures,
				const std::string& textureID,
				float edgeSize = 0.20f);
			
			void buildMesh(float size_x, float size_y);
			floats4& color() { return m_color; }

			Frame& edge_size(float v) { m_edgeSize = v; return *this; }
			Frame& color(floats4 v) { m_color = v; return *this; }

			virtual void update(float) override;

			virtual void onInput(rynx::mapped_input&) override {}
			virtual void draw(rynx::MeshRenderer& meshRenderer, TextRenderer&) const override;
		};
	}
}