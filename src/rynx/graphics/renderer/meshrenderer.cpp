
#include "meshrenderer.hpp"
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/camera/Camera.hpp>

rynx::MeshRenderer::MeshRenderer(
	std::shared_ptr<GPUTextures> textures,
	std::shared_ptr<rynx::graphics::Shaders> shaders
)
{
	m_textures = textures;
	m_shaders = shaders;
	m_meshes = std::make_shared<mesh_collection>(m_textures);
	init();
}

static constexpr int32_t InstancesPerDrawCall = 1024 * 10;

void rynx::MeshRenderer::init() {
	glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
	glClearDepth(1);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	{
		m_shader2d = m_shaders->load_shader("renderer2d", "../shaders/2d_shader.vs.glsl", "../shaders/2d_shader.fs.glsl");
		m_shader2d->activate();
		
		m_modelUniform = m_shader2d->uniform("model");
		m_viewUniform = m_shader2d->uniform("view");
		m_projectionUniform = m_shader2d->uniform("projection");
		m_colorUniform = m_shader2d->uniform("color");
		m_shader2d->uniform("tex", 0);
	}

	{
		m_shader2d_instanced = m_shaders->load_shader("renderer2d_instanced", "../shaders/2d_shader_instanced.vs.glsl", "../shaders/2d_shader_instanced.fs.glsl");
		m_shader2d_instanced->activate();

		m_viewUniform_instanced = m_shader2d_instanced->uniform("view");
		m_projectionUniform_instanced = m_shader2d_instanced->uniform("projection");
		m_shader2d_instanced->uniform("tex", 0);
	}

	{
		glGenBuffers(1, &model_matrices_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);
		glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

		glGenBuffers(1, &colors_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
		glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	}

	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}

void rynx::MeshRenderer::loadDefaultMesh(const std::string& textureName) {
	m_rectangle = m_meshes->create("empty_box", Shape::makeBox(2.f), textureName);
}

void rynx::MeshRenderer::setDepthTest(bool depthTestEnabled)
{
	if (depthTestEnabled)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void rynx::MeshRenderer::clearScreen() {
	/*
	float depth_clear_value = 1.0f;
	glClearBufferfv(GL_DEPTH, 0, &depth_clear_value);
	
	float color_clear[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, color_clear);
	glClearBufferfv(GL_COLOR, 1, color_clear);
	*/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}

void rynx::MeshRenderer::setCamera(std::shared_ptr<Camera> camera) {
	m_pCamera = camera;
}

void rynx::MeshRenderer::drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const floats4& color) {
	matrix4 model;
	model.identity();
	drawLine(p1, p2, model, width, color);
}

void rynx::MeshRenderer::drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const floats4& color) {
	vec3<float> mid = (p1 + p2) * 0.5f;

	matrix4 model_;
	model_.discardSetTranslate(mid.x, mid.y, mid.z);
	model_.rotate_2d(math::atan_approx((p1.y - p2.y) / (p1.x - p2.x)));
	// model_.rotate(math::atan_approx((p1.y - p2.y) / (p1.x - p2.x)), 0, 0, 1);
	model_.scale((p1 - p2).length() * 0.5f, width * 0.5f, 1.0f);
	model_ *= model;

	floats4 limits = m_textures->textureLimits("Empty");
	float botCoordX = limits.data[0];
	float botCoordY = limits.data[1];
	float topCoordX = limits.data[2];
	float topCoordY = limits.data[3];

	m_rectangle->texCoords.clear();
	m_rectangle->putUVCoord(topCoordX, botCoordY);
	m_rectangle->putUVCoord(topCoordX, topCoordY);
	m_rectangle->putUVCoord(botCoordX, topCoordY);
	m_rectangle->putUVCoord(botCoordX, botCoordY);

	m_rectangle->bind();
	m_rectangle->rebuildTextureBuffer();

	drawMesh(*m_rectangle, model_, "Empty", color);
}

void rynx::MeshRenderer::drawRectangle(const matrix4& model, const std::string& texture, const floats4& color) {
	floats4 limits = m_textures->textureLimits(texture);
	float botCoordX = limits.data[0];
	float botCoordY = limits.data[1];
	float topCoordX = limits.data[2];
	float topCoordY = limits.data[3];

	m_rectangle->texCoords.clear();
	m_rectangle->putUVCoord(topCoordX, botCoordY);
	m_rectangle->putUVCoord(topCoordX, topCoordY);
	m_rectangle->putUVCoord(botCoordX, topCoordY);
	m_rectangle->putUVCoord(botCoordX, botCoordY);

	m_rectangle->bind();
	m_rectangle->rebuildTextureBuffer();
	
	drawMesh(*m_rectangle, model, texture, color);
}

void rynx::MeshRenderer::cameraToGPU() {
	m_shader2d->activate();
	glUniformMatrix4fv(m_viewUniform, 1, GL_FALSE, m_pCamera->getView().m);
	glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, m_pCamera->getProjection().m);

	m_shader2d_instanced->activate();
	glUniformMatrix4fv(m_viewUniform_instanced, 1, GL_FALSE, m_pCamera->getView().m);
	glUniformMatrix4fv(m_projectionUniform_instanced, 1, GL_FALSE, m_pCamera->getProjection().m);
}

void rynx::MeshRenderer::drawMesh(const Mesh& mesh, const matrix4& model, const std::string& texture, const floats4& color) {
	m_shader2d->activate();
	mesh.bind();

	m_textures->bindTexture(0, texture);	
	glUniformMatrix4fv(m_modelUniform, 1, GL_FALSE, model.m);
	glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);

	glDrawElements(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_SHORT, 0);
	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}

void rynx::MeshRenderer::drawMeshInstanced(const Mesh& mesh, const std::string& texture, const std::vector<matrix4>& models, const std::vector<floats4>& colors) {
	m_shader2d_instanced->activate();
	mesh.bind();
	m_textures->bindTexture(0, texture);

	{
		const GLuint model_matrix_slot = 2;
		glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);
		for (int i = 0; i < 4; ++i) {
			glVertexAttribPointer(model_matrix_slot + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(i * 4 * sizeof(float)));
			glEnableVertexAttribArray(model_matrix_slot + i);
			glVertexAttribDivisor(model_matrix_slot + i, 1); // model matrices, one per entity -> 1
		}
	}

	{
		const GLuint colors_slot = 6;
		glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
		glVertexAttribPointer(colors_slot, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
		glEnableVertexAttribArray(colors_slot);
		glVertexAttribDivisor(colors_slot, 1); // colors, use one per entity -> 1
	}

	// If divisor is zero, the attribute at slot index advances once per vertex
	glVertexAttribDivisor(0, 0); // vertex positions are per vertex data -> 0
	glVertexAttribDivisor(1, 0); // texture coordinates are per vertex data -> 0

	size_t currentIteration = 0;
	while (models.size() > currentIteration * InstancesPerDrawCall) {
		int32_t remaining = static_cast<int32_t>(models.size() - currentIteration * InstancesPerDrawCall);
		int32_t instances_for_current_iteration = remaining > InstancesPerDrawCall ? InstancesPerDrawCall : remaining;

		{
			const GLuint model_matrix_slot = 2;
			glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);

			// Buffer orphaning, a common way to improve streaming perf for some reason.
			glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, instances_for_current_iteration * 4 * 4 * sizeof(GLfloat), models.data() + currentIteration * InstancesPerDrawCall);
		}

		{
			const GLuint colors_slot = 6;
			glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
			
			// Buffer orphaning, a common way to improve streaming perf for some reason.
			glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, instances_for_current_iteration * 4 * sizeof(GLfloat), colors.data() + currentIteration * InstancesPerDrawCall);
		}

		glDrawElementsInstanced(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_SHORT, 0, instances_for_current_iteration);
		++currentIteration;
	}

	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}