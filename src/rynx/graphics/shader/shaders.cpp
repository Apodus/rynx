
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/opengl.hpp>
#include <rynx/system/assert.hpp>

#include <string>

rynx::graphics::Shaders::Shaders() {
	GLint n;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n);
	logmsg("Max geometry shader output vertices: %d", n);
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &n);
	logmsg("Max geometry shader output components: %d", n);
}

rynx::graphics::Shaders::~Shaders() {
	release();
}

void rynx::graphics::Shaders::release() {
	m_shaders.clear();
}

std::shared_ptr<rynx::graphics::Shader> rynx::graphics::Shaders::activate_shader(const std::string& name) {
	if(m_activeShaderName == name)
		return m_activeShader;

	if(m_activeShader)
		m_activeShader->stop();
	
	auto it = m_shaders.find(name);
	rynx_assert(it != m_shaders.end(), "trying to start a shader that has not been loaded: '%s'", name.c_str());
	m_activeShaderName = name;
	m_activeShader = it->second;
	m_activeShader->activate();
	
	return m_activeShader;
}

std::shared_ptr<rynx::graphics::Shader> rynx::graphics::Shaders::load_shader(
	const std::string& name,
	const std::string& vertexShader,
	const std::string& fragmentShader)
{
	logmsg("loading shader %s", name.c_str());
	auto pShader = std::make_shared<Shader>(name, vertexShader, fragmentShader);
	pShader->set_shader_manager(this);
	m_shaders.insert_or_assign(name, pShader);
	return pShader;
}
