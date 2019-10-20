
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/tech/math/matrix.hpp>
#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/color.hpp>

#include <memory>

class GPUTextures;
class Mesh;

namespace rynx {
	namespace menu {
		class Frame : public Component {

			vec4<float> m_color;
			vec3<float> m_prevScale;
			std::string m_textureID;
			matrix4 m_modelMatrix;
			std::unique_ptr<Mesh> m_backgroundMesh;
			float m_edgeSize;

			void initMesh(GPUTextures& textures);

		public:
			Frame(
				Component* parent,
				GPUTextures& textures,
				const std::string& textureID,
				float edgeSize = 0.20f);
			
			void buildMesh(float size_x, float size_y);
			vec4<float>& color() { return m_color; }

			Frame& edge_size(float v) { m_edgeSize = v; return *this; }
			Frame& color(vec4<float> v) { m_color = v; return *this; }

			virtual void update(float) override;

			virtual void onInput(rynx::input::mapped_input&) override {}
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer&) const override;
		};
	}
}