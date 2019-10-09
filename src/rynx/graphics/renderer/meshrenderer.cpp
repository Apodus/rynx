
#include "meshrenderer.hpp"
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/camera/Camera.hpp>

MeshRenderer::MeshRenderer(
	std::shared_ptr<GPUTextures> textures,
	std::shared_ptr<Shaders> shaders
)
{
	m_textures = textures;
	m_shaders = shaders;
	init();
}

MeshRenderer::~MeshRenderer() {

}

void MeshRenderer::init() {
	glClearColor(0.2f, 0.3f, 0.5f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
	glClearDepth(1);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_rectangle = PolygonTesselator<vec3<float>>().tesselate(Shape::makeBox(2.f));
	m_rectangle->build();

	m_line = PolygonTesselator<vec3<float>>().tesselate(Shape::makeBox(1.f));
	m_line->build();
	
	std::shared_ptr<Shader> shader2d = m_shaders->loadShader("renderer2d", "../shaders/2d_shader_130.vs.glsl", "../shaders/2d_shader_130.fs.glsl");
	m_shaders->switchToShader("renderer2d");

	m_modelUniform = shader2d->uniform("model");
	m_viewUniform  = shader2d->uniform("view");
	m_projectionUniform = shader2d->uniform("projection");
	m_colorUniform = shader2d->uniform("color");

	m_texSamplerUniform = shader2d->uniform("tex");
	glUniform1i(m_texSamplerUniform, 0);
}

void MeshRenderer::setDepthTest(bool depthTestEnabled)
{
	if (depthTestEnabled)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void MeshRenderer::clearScreen() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MeshRenderer::setCamera(std::shared_ptr<Camera> camera) {
	m_pCamera = camera;
}

void MeshRenderer::drawLine(const vec3<float>& p1, const vec3<float>& p2, float width, const vec4<float>& color) {
	drawLine(p1, p2, matrix4(), width, color);
}

void MeshRenderer::drawLine(const vec3<float>& p1, const vec3<float>& p2, const matrix4& model, float width, const vec4<float>& color) {
	matrix4 model_;
	vec3<float> mid = (p1 + p2) * 0.5f;
	model_.discardSetTranslate(mid.x, mid.y, mid.z);
	model_.rotate(::atan((p1.y - p2.y) / (p1.x - p2.x)), 0, 0, 1);
	model_.scale((p1 - p2).lengthApprox(), width, 1.0f);
	model_ *= model;

	const vec4<float>& limits = m_textures->textureLimits("Empty");
	float botCoordX = limits.data[0];
	float botCoordY = limits.data[1];
	float topCoordX = limits.data[2];
	float topCoordY = limits.data[3];

	m_line->texCoords.clear();
	m_line->putUVCoord(topCoordX, botCoordY);
	m_line->putUVCoord(topCoordX, topCoordY);
	m_line->putUVCoord(botCoordX, topCoordY);
	m_line->putUVCoord(botCoordX, botCoordY);

	m_line->bind();
	m_line->rebuildTextureBuffer();

	drawMesh(*m_line, model_, "Empty", color);
}

void MeshRenderer::drawRectangle(const matrix4& model, const std::string& texture, const vec4<float>& color) {
	const vec4<float>& limits = m_textures->textureLimits(texture);
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

bool MeshRenderer::verifyNoGLErrors() const {
	auto kek = glGetError();
	if (kek == GL_NO_ERROR)
		return true;
	logmsg("gl error: %d", kek);
	return false;
}

void MeshRenderer::cameraToGPU() {
	m_shaders->switchToShader("renderer2d");
	glUniformMatrix4fv(m_viewUniform, 1, GL_FALSE, m_pCamera->getView().data);
	glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, m_pCamera->getProjection().data);
}

void MeshRenderer::drawMesh(const Mesh& mesh, const matrix4& model, const std::string& texture, const vec4<float>& color) {
	m_shaders->switchToShader("renderer2d");
	mesh.bind();
	m_textures->bindTexture(0, texture);
	glUniformMatrix4fv(m_modelUniform, 1, GL_FALSE, model.data);
	glUniform4f(m_colorUniform, color.r, color.g, color.b, color.a);
	glDrawElements(GL_TRIANGLES, mesh.getIndexCount(), GL_UNSIGNED_SHORT, 0);
}