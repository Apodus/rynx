
#pragma once

#include <rynx/graphics/shader/shader.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <string>
#include <memory>

typedef int GLint;
typedef unsigned GLuint;

class Shaders
{
public:
	Shaders();
	~Shaders();

	void release();

	std::shared_ptr<Shader> load_shader(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader);
	std::shared_ptr<Shader> activate_shader(const std::string& name);

private:
	rynx::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
	
	std::shared_ptr<Shader> m_activeShader;
	std::string m_activeShaderName;
};