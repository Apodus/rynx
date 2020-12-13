#pragma once

#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/math/matrix.hpp>

#include <memory>

#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/geometry/polygon_triangulation.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp>

namespace rynx {
	class camera;
	
	namespace graphics {
		class mesh;
		class text_renderer;
		class renderable_text;

		class mesh_collection {
		public:
			mesh_collection(std::shared_ptr<rynx::graphics::GPUTextures> gpuTextures) : m_pGpuTextures(gpuTextures) {}

			mesh* get(const std::string& s) {
				auto it = m_storage.find(s);
				rynx_assert(it != m_storage.end(), "mesh not found: %s", s.c_str());
				return it->second.get();
			}

			mesh* create(const std::string& s, std::unique_ptr<mesh> mesh, std::string texture_name) {
				auto it = m_storage.emplace(s, std::move(mesh));
				it.first->second->build();
				it.first->second->texture_name = texture_name;
				return it.first->second.get();
			}

			void erase(const std::string& s) {
				m_storage.erase(s);
			}

			mesh* create(const std::string& s, polygon shape, const std::string& texture) {
				auto it = m_storage.emplace(s, rynx::polygon_triangulation().make_mesh(shape, m_pGpuTextures->textureLimits(texture)));
				it.first->second->build();
				it.first->second->texture_name = texture;
				return it.first->second.get();
			}

		private:
			std::shared_ptr<rynx::graphics::GPUTextures> m_pGpuTextures;
			rynx::unordered_map<std::string, std::unique_ptr<mesh>> m_storage;
		};

		class renderer {
			mesh* m_rectangle;
			std::unique_ptr<rynx::graphics::text_renderer> m_pTextRenderer;

			std::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			std::shared_ptr<rynx::graphics::shaders> m_shaders;
			std::shared_ptr<camera> m_pCamera;

			std::shared_ptr<mesh_collection> m_meshes;

			std::shared_ptr<rynx::graphics::shader> shader_single;
			std::shared_ptr<rynx::graphics::shader> shader_instanced;
			std::shared_ptr<rynx::graphics::shader> shader_instanced_deferred;

			GLuint model_matrices_buffer;
			GLuint colors_buffer;
			GLuint normals_buffer;

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
				DrawType type);

		public:
			renderer(std::shared_ptr<rynx::graphics::GPUTextures> texture, std::shared_ptr<rynx::graphics::shaders> shaders);

			void setDepthTest(bool depthTestEnabled);
			std::shared_ptr<mesh_collection> meshes() const { return m_meshes; }

			void loadDefaultMesh(const std::string& textureName);
			void cameraToGPU();
			void setCamera(std::shared_ptr<camera> camera);
			void drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const floats4& color);
			void drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const floats4& color);
			void drawRectangle(const matrix4& model, const std::string& texture_name, const floats4& color = floats4(1, 1, 1, 1));

			void drawMesh(const mesh& mesh, const matrix4& model, const floats4& color = floats4(1, 1, 1, 1));
			void drawMesh(const std::string& mesh_name, const matrix4& model, const floats4& color = floats4(1, 1, 1, 1)) {
				drawMesh(*m_meshes->get(mesh_name), model, color);
			}

			void drawText(const rynx::graphics::renderable_text& text);

			void drawMeshInstanced(const mesh& mesh, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
				instanced_draw_impl(mesh, models.size(), models.data(), colors.data(), DrawType::Forward);
			}
			void drawMeshInstanced(const std::string& mesh_name, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
				drawMeshInstanced(*m_meshes->get(mesh_name), models, colors);
			}

			void drawMeshInstancedDeferred(const mesh& mesh, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
				instanced_draw_impl(mesh, models.size(), models.data(), colors.data(), DrawType::Deferred);
			}
			void drawMeshInstancedDeferred(const std::string& mesh_name, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
				drawMeshInstancedDeferred(*m_meshes->get(mesh_name), models, colors);
			}
			void drawMeshInstancedDeferred(const mesh& mesh, size_t num, const matrix4* models, const floats4* colors) {
				instanced_draw_impl(mesh, num, models, colors, DrawType::Deferred);
			}

		private:
			void init();
		};
	}
}