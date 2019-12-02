
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/opengl.hpp>
#include <rynx/system/assert.hpp>

#include <sstream>
#include <string>
#include <stdexcept>

Shaders::Shaders() {
	GLint n;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n);
	logmsg("Max geometry shader output vertices: %d", n);
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &n);
	logmsg("Max geometry shader output components: %d", n);
}

Shaders::~Shaders() {
	release();
}

void Shaders::release() {
	m_shaders.clear();
}

std::shared_ptr<Shader> Shaders::switchToShader(const std::string& name) {
	if(m_activeShaderName == name)
		return m_activeShader;

	if(m_activeShader)
		m_activeShader->stop();

	auto it = m_shaders.find(name);
	rynx_assert(it != m_shaders.end(), "trying to start a shader that has not been loaded: '%s'", name.c_str());
	m_activeShader = it->second;
	m_activeShader->start();
	m_activeShaderName = name;
	return m_activeShader;
}

std::shared_ptr<Shader> Shaders::loadShader(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader) {
	logmsg("loading shader %s", name.c_str());
	std::shared_ptr<Shader> pShader(new Shader(vertexShader, fragmentShader));
	m_shaders[name] = pShader;
	return pShader;
}

GLuint Shaders::operator[](const std::string& program_name) const {
	auto it = m_shaders.find(program_name);
	rynx_assert(it != m_shaders.end(), "shader program '%s' not found!", program_name.c_str());
	return it->second->get_program();
}

std::shared_ptr<Shader> Shaders::operator()(const std::string& program_name) {
	auto it = m_shaders.find(program_name);
	rynx_assert(it != m_shaders.end(), "shader program '%s' not found!", program_name.c_str());
	return it->second;
}

