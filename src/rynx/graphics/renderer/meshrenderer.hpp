#pragma once

#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/tech/math/matrix.hpp>

#include <memory>

class Camera;
class Mesh;

#include <rynx/tech/unordered_map.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>

namespace rynx {
	class mesh_collection {
	public:
		mesh_collection(std::shared_ptr<GPUTextures> gpuTextures) : m_pGpuTextures(gpuTextures) {}

		Mesh* get(const std::string& s) {
			auto it = m_storage.find(s);
			rynx_assert(it != m_storage.end(), "mesh not found: %s", s.c_str());
			return it->second.get();
		}

		Mesh* create(const std::string& s, Polygon<vec3<float>> shape, const std::string& texture) {
			auto it = m_storage.emplace(s, PolygonTesselator<vec3<float>>().tesselate(shape, m_pGpuTextures->textureLimits(texture)));
			it.first->second->build();
			return it.first->second.get();
		}

	private:
		std::shared_ptr<GPUTextures> m_pGpuTextures;
		rynx::unordered_map<std::string, std::unique_ptr<Mesh>> m_storage;
	};

	class MeshRenderer {
		Mesh* m_rectangle;
		
		std::shared_ptr<GPUTextures> m_textures;
		std::shared_ptr<rynx::graphics::Shaders> m_shaders;
		std::shared_ptr<Camera> m_pCamera;

		std::shared_ptr<mesh_collection> m_meshes;

		std::shared_ptr<rynx::graphics::Shader> shader_single;
		std::shared_ptr<rynx::graphics::Shader> shader_instanced;
		std::shared_ptr<rynx::graphics::Shader> shader_instanced_deferred;

		GLuint model_matrices_buffer;
		GLuint colors_buffer;
		GLuint normals_buffer;

		bool verifyNoGLErrors() const;

		enum class DrawType {
			Forward,
			Deferred
		};

		void instanced_draw_impl(
			const Mesh& mesh,
			const std::string& texture,
			const std::vector<matrix4>& models,
			const std::vector<floats4>& colors,
			DrawType type);

	public:
		MeshRenderer(std::shared_ptr<GPUTextures> texture, std::shared_ptr<rynx::graphics::Shaders> shaders);

		void setDepthTest(bool depthTestEnabled);
		std::shared_ptr<mesh_collection> meshes() const { return m_meshes; }

		void loadDefaultMesh(const std::string& textureName);
		void clearScreen();
		void cameraToGPU();
		void setCamera(std::shared_ptr<Camera> camera);
		void drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const floats4& color);
		void drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const floats4& color);
		void drawRectangle(const matrix4& model, const std::string& texture, const floats4& color = floats4(1, 1, 1, 1));
		
		void drawMesh(const Mesh& mesh, const matrix4& model, const std::string& texture, const floats4& color = floats4(1, 1, 1, 1));
		void drawMesh(const std::string& mesh_name, const matrix4& model, const std::string& texture, const floats4& color = floats4(1, 1, 1, 1)) {
			drawMesh(*m_meshes->get(mesh_name), model, texture, color);
		}

		void drawMeshInstanced(const Mesh& mesh, const std::string& texture, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
			instanced_draw_impl(mesh, texture, models, colors, DrawType::Forward);
		}
		void drawMeshInstanced(const std::string& mesh_name, const std::string& texture, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
			drawMeshInstanced(*m_meshes->get(mesh_name), texture, models, colors);
		}

		void drawMeshInstancedDeferred(const Mesh& mesh, const std::string& texture, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
			instanced_draw_impl(mesh, texture, models, colors, DrawType::Deferred);
		}
		void drawMeshInstancedDeferred(const std::string& mesh_name, const std::string& texture, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
			drawMeshInstancedDeferred(*m_meshes->get(mesh_name), texture, models, colors);
		}

	private:
		void init();
	};
}