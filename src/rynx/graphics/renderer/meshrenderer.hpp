#pragma once

#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/graphics/mesh/collection.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/memory.hpp>
#include <rynx/math/geometry/polygon_triangulation.hpp>
#include <rynx/math/matrix.hpp>

#include <memory>

namespace rynx {
	class camera;
	
	namespace graphics {
		class mesh;
		class text_renderer;
		class renderable_text;

		class renderer {
			mesh_id m_rectangle;
			std::unique_ptr<rynx::graphics::text_renderer> m_pTextRenderer;

			std::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			std::shared_ptr<rynx::graphics::shaders> m_shaders;
			rynx::observer_ptr<camera> m_pCamera;

			std::shared_ptr<mesh_collection> m_meshes;

			std::shared_ptr<rynx::graphics::shader> shader_single;
			std::shared_ptr<rynx::graphics::shader> shader_instanced;
			std::shared_ptr<rynx::graphics::shader> shader_instanced_deferred;

			GLuint model_matrices_buffer = 0;
			GLuint colors_buffer = 0;
			GLuint normals_buffer = 0;
			GLuint tex_id_buffer = 0;

			bool verifyNoGLErrors() const;

			enum class DrawType {
				Forward,
				Deferred
			};

			void instanced_draw_impl(
				const mesh& mesh,
				size_t num_instances,
				const matrix4* models,
				const floats4* colors,
				const rynx::graphics::texture_id* tex_ids,
				DrawType type);

		public:
			renderer(std::shared_ptr<rynx::graphics::GPUTextures> texture, std::shared_ptr<rynx::graphics::shaders> shaders);

			void setDepthTest(bool depthTestEnabled);
			std::shared_ptr<mesh_collection> meshes() const { return m_meshes; }

			void cameraToGPU();
			void setCamera(rynx::observer_ptr<camera> camera);
			void setDefaultFont(const Font& font) { m_pTextRenderer->setDefaultFont(font); }
			const Font& getDefaultFont() const { return m_pTextRenderer->getDefaultFont(); }

			void drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const floats4& color);
			void drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const floats4& color);
			void drawRectangle(
				const matrix4& model,
				rynx::graphics::texture_id tex_id,
				const floats4& color = floats4(1, 1, 1, 1)
			);

			void drawMesh(const mesh& mesh, rynx::graphics::texture_id texture_id, const matrix4& model, const floats4& color = floats4(1, 1, 1, 1));
			void drawMesh(mesh_id id, rynx::graphics::texture_id texture_id, const matrix4& model, const floats4& color = floats4(1, 1, 1, 1)) {
				drawMesh(*m_meshes->get(id), texture_id, model, color);
			}

			void drawText(const rynx::graphics::renderable_text& text);

			void drawMeshInstanced(
				const mesh& mesh,
				const std::vector<matrix4>& models,
				const std::vector<floats4>& colors,
				const std::vector<rynx::graphics::texture_id>& tex_data)
			{
				instanced_draw_impl(mesh, models.size(), models.data(), colors.data(), tex_data.data(), DrawType::Forward);
			}
			
			void drawMeshInstanced(
				mesh_id id,
				const std::vector<matrix4>& models,
				const std::vector<floats4>& colors,
				const std::vector<rynx::graphics::texture_id>& tex_data)
			{
				drawMeshInstanced(*m_meshes->get(id), models, colors, tex_data);
			}

			void drawMeshInstancedDeferred(
				const mesh& mesh,
				const std::vector<matrix4>& models,
				const std::vector<floats4>& colors,
				const std::vector<rynx::graphics::texture_id>& tex_data)
			{
				instanced_draw_impl(mesh, models.size(), models.data(), colors.data(), tex_data.data(), DrawType::Deferred);
			}
			
			void drawMeshInstancedDeferred(
				mesh_id id,
				const std::vector<matrix4>& models,
				const std::vector<floats4>& colors,
				const std::vector<rynx::graphics::texture_id>& tex_data)
			{
				drawMeshInstancedDeferred(*m_meshes->get(id), models, colors, tex_data);
			}
			
			void drawMeshInstancedDeferred(
				const mesh& mesh,
				size_t num,
				const matrix4* models,
				const floats4* colors,
				const rynx::graphics::texture_id* tex_data)
			{
				instanced_draw_impl(mesh, num, models, colors, tex_data, DrawType::Deferred);
			}

		private:
			void init();
		};
	}
}