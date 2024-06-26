
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/internal/includes.hpp>
#include <rynx/std/string.hpp>
#include <rynx/system/assert.hpp>
#include <rynx/filesystem/virtual_filesystem.hpp>

rynx::graphics::shaders::shaders(rynx::observer_ptr<rynx::filesystem::vfs> vfs) {
	GLint n;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n);
	logmsg("Max geometry shader output vertices: %d", n);
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &n);
	logmsg("Max geometry shader output components: %d", n);
	
	m_vfs = vfs;
}

rynx::graphics::shaders::~shaders() {
	release();
}

void rynx::graphics::shaders::release() {
	m_shaders.clear();
}

rynx::shared_ptr<rynx::graphics::shader> rynx::graphics::shaders::activate_shader(const rynx::string& name) {
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

rynx::shared_ptr<rynx::graphics::shader> rynx::graphics::shaders::load_shader(
	const rynx::string& name,
	const rynx::string& vertexShaderPath,
	const rynx::string& fragmentShaderPath)
{
	logmsg("loading shader %s", name.c_str());
	
	std::vector<char> vertexShaderContent = m_vfs->open_read(vertexShaderPath)->read_all();
	std::vector<char> fragmentShaderContent = m_vfs->open_read(fragmentShaderPath)->read_all();
	vertexShaderContent.push_back('\0');
	fragmentShaderContent.push_back('\0');

	auto pShader = rynx::make_shared<shader>(name, vertexShaderContent.data(), fragmentShaderContent.data());
	pShader->set_shader_manager(this);
	m_shaders.insert_or_assign(name, pShader);
	return pShader;
}
