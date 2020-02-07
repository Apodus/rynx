
#include "meshrenderer.hpp"
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/camera/camera.hpp>

rynx::MeshRenderer::MeshRenderer(
	std::shared_ptr<GPUTextures> textures,
	std::shared_ptr<rynx::graphics::shaders> shaders
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
		shader_single = m_shaders->load_shader("renderer2d", "../shaders/2d_shader.vs.glsl", "../shaders/2d_shader.fs.glsl");
		shader_instanced = m_shaders->load_shader("renderer2d_instanced", "../shaders/2d_shader_instanced.vs.glsl", "../shaders/2d_shader_instanced.fs.glsl");
		shader_instanced_deferred = m_shaders->load_shader("renderer2d_instanced_deferred", "../shaders/2d_shader_instanced_deferred.vs.glsl", "../shaders/2d_shader_instanced_deferred.fs.glsl");

		shader_single->activate();
		shader_single->uniform("tex", 0);
		
		shader_instanced->activate();
		shader_instanced->uniform("tex", 0);
		
		shader_instanced_deferred->activate();
		shader_instanced_deferred->uniform("tex", 0);
	}

	{
		glGenBuffers(1, &model_matrices_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);
		glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

		glGenBuffers(1, &colors_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
		glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

		glGenBuffers(1, &normals_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, normals_buffer);
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
	auto set_camera_to_shader = [this](rynx::graphics::shader& shader) {
		shader.activate();
		shader.uniform("view", m_pCamera->getView());
		shader.uniform("projection", m_pCamera->getProjection());
	};

	set_camera_to_shader(*shader_single);
	set_camera_to_shader(*shader_instanced);
	set_camera_to_shader(*shader_instanced_deferred);
}

void rynx::MeshRenderer::drawMesh(const Mesh& mesh, const matrix4& model, const std::string& texture, const floats4& color) {
	shader_single->activate();
	mesh.bind();

	m_textures->bindTexture(0, texture);	
	shader_single->uniform("model", model);
	shader_single->uniform("color", color);
	
	glDrawElements(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_SHORT, 0);
	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}

void rynx::MeshRenderer::instanced_draw_impl(
	const Mesh& mesh,
	const std::string& texture,
	size_t num_instances,
	const matrix4* models,
	const floats4* colors,
	DrawType type) {
	
	rynx::graphics::shader* shader = nullptr;
	if (type == DrawType::Forward) {
		shader = shader_instanced.get();
	}
	else if (type == DrawType::Deferred) {
		shader = shader_instanced_deferred.get();
	}

	shader->activate();

	mesh.bind();
	m_textures->bindTexture(0, texture);

	{
		const GLuint model_matrix_slot = shader->attribute("model");
		glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);
		for (int i = 0; i < 4; ++i) {
			glVertexAttribPointer(model_matrix_slot + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(i * 4 * sizeof(float)));
			glEnableVertexAttribArray(model_matrix_slot + i);
			glVertexAttribDivisor(model_matrix_slot + i, 1); // model matrices, one per entity -> 1
		}
	}

	{
		const GLuint colors_slot = shader->attribute("color");
		glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
		glVertexAttribPointer(colors_slot, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
		glEnableVertexAttribArray(colors_slot);
		glVertexAttribDivisor(colors_slot, 1); // colors, use one per entity -> 1
	}

	// If divisor is zero, the attribute at slot index advances once per vertex
	glVertexAttribDivisor(0, 0); // vertex positions are per vertex data -> 0
	glVertexAttribDivisor(1, 0); // texture coordinates are per vertex data -> 0

	size_t currentIteration = 0;
	while (num_instances > currentIteration* InstancesPerDrawCall) {
		int32_t remaining = static_cast<int32_t>(num_instances - currentIteration * InstancesPerDrawCall);
		int32_t instances_for_current_iteration = remaining > InstancesPerDrawCall ? InstancesPerDrawCall : remaining;

		{
			glBindBuffer(GL_ARRAY_BUFFER, model_matrices_buffer);
			glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, instances_for_current_iteration * 4 * 4 * sizeof(GLfloat), models + currentIteration * InstancesPerDrawCall);
		}

		{
			glBindBuffer(GL_ARRAY_BUFFER, colors_buffer);
			glBufferData(GL_ARRAY_BUFFER, InstancesPerDrawCall * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, instances_for_current_iteration * 4 * sizeof(GLfloat), colors + currentIteration * InstancesPerDrawCall);
		}

		glDrawElementsInstanced(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_SHORT, 0, instances_for_current_iteration);
		++currentIteration;
	}

	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
}