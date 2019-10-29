#pragma once

#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/tech/math/matrix.hpp>

#include <memory>

class Camera;
class Mesh;

class MeshRenderer {
	std::unique_ptr<Mesh> m_rectangle;
	std::unique_ptr<Mesh> m_line;

	std::shared_ptr<GPUTextures> m_textures;
	std::shared_ptr<Shaders> m_shaders;
	std::shared_ptr<Camera> m_pCamera;

	GLuint m_modelUniform, m_viewUniform, m_projectionUniform;
	GLuint m_texSamplerUniform, m_colorUniform;

	GLuint m_viewUniform_instanced;
	GLuint m_projectionUniform_instanced;
	GLuint m_texSamplerUniform_instanced;

	// buffer for model matrices, for instanced rendering.
	GLuint model_matrices_buffer;

	bool verifyNoGLErrors() const;

public:
	MeshRenderer(std::shared_ptr<GPUTextures> texture, std::shared_ptr<Shaders> shaders);
	~MeshRenderer();

	void setDepthTest(bool depthTestEnabled);

	void clearScreen();
	void cameraToGPU();
	void setCamera(std::shared_ptr<Camera> camera);
	void drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const vec4<float>& color);
	void drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const vec4<float>& color);
	void drawRectangle(const matrix4& model, const std::string& texture, const vec4<float>& color = vec4<float>(1, 1, 1, 1));
	void drawMesh(const Mesh& mesh, const matrix4& model, const std::string& texture, const vec4<float>& color = vec4<float>(1, 1, 1, 1));
	void drawMeshInstanced(const Mesh& mesh, const std::string& texture, const std::vector<matrix4>& models, const vec4<float> color = vec4<float>(1, 1, 1, 1));

private:
	void init();
};