
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/shader/shader.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/system/assert.hpp>
#include <rynx/tech/math/matrix.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace {
	std::string readFile(const std::string& path)
	{
		std::stringstream ss;
		std::ifstream file(path);
		ss << file.rdbuf();
		file.close();
		rynx_assert(ss.str().length() > 0, "Shader source code length zero!");

		return ss.str();
	}

	void printlogmsg(GLuint obj)
	{
		int infologLength = 0;
		char infoLog[1024];

		if (glIsShader(obj))
			glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
		else
			glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);

		if (infoLog[0])
		{
			logmsg("Shader build info: %s", infoLog);
		}
	}

	GLuint loadVertexShader(const std::string& filename)
	{
		std::string str = readFile(filename);
		const char* data = str.c_str();

		GLuint ret = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(ret, 1, &data, nullptr);
		glCompileShader(ret);

		return ret;
	}

	GLuint loadFragmentShader(const std::string& filename)
	{
		std::string str = readFile(filename);
		const char* data = str.c_str();

		GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(ret, 1, &data, nullptr);
		glCompileShader(ret);

		return ret;
	}

	GLuint loadGeometryShader(const std::string& filename)
	{
		std::string str = readFile(filename);
		const char* data = str.c_str();

		GLuint ret = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(ret, 1, &data, nullptr);
		glCompileShader(ret);

		return ret;
	}
}

rynx::graphics::shader::shader(
	std::string name,
	const std::string& vert,
	const std::string& frag)
	:
	m_name(std::move(name)),
	m_programID(0),
	m_vertexID(0),
	m_geometryID(0),
	m_fragmentID(0),
	m_started(false)
{
	rynx_assert(!vert.empty(), "trying to open vertex shader file without name?");
	rynx_assert(!frag.empty(), "trying to open fragment shader file without name?");

	m_vertexID = loadVertexShader(vert); printlogmsg(m_vertexID);
	m_fragmentID = loadFragmentShader(frag); printlogmsg(m_fragmentID);

	rynx_assert(m_vertexID != 0, "Loading vertex shader failed: %s", vert.c_str());
	rynx_assert(m_fragmentID != 0, "Loading fragment shader failed: %s", frag.c_str());

	m_programID = glCreateProgram(); printlogmsg(m_programID);
	rynx_assert(m_programID != 0, "Creating shader program failed");

	glAttachShader(m_programID, m_vertexID);
	glAttachShader(m_programID, m_fragmentID);
	glLinkProgram(m_programID);

	for (;;) {
		auto errorValue = glGetError();
		if (errorValue == GL_NO_ERROR)
			break;
		rynx_assert(false, "gl error value: %d", errorValue);
	}
	printlogmsg(m_programID);
}

rynx::graphics::shader::shader(
	std::string name,
	const std::string& vert,
	const std::string& frag,
	const std::string& geom,
	GLint input,
	GLint output,
	GLint vertices)
	:
	m_name(std::move(name)),
	m_programID(0),
	m_vertexID(0),
	m_geometryID(0),
	m_fragmentID(0),
	m_started(false)
{
	rynx_assert(!vert.empty(), "Creating shader: vertex program file name is empty");
	rynx_assert(!geom.empty(), "Creating shader: geometry program file name is empty");
	rynx_assert(!frag.empty(), "Creating shader: fragment program file name is empty");

	m_vertexID = loadVertexShader(vert);
	m_geometryID = loadGeometryShader(geom);
	m_fragmentID = loadFragmentShader(frag);

	rynx_assert(m_vertexID != 0, "creating vertex program failed");
	rynx_assert(m_geometryID != 0, "creating geometry program failed");
	rynx_assert(m_fragmentID != 0, "creating fragment program failed");

	m_programID = glCreateProgram();
	rynx_assert(m_programID != 0, "creating shader program failed");

	glAttachShader(m_programID, m_vertexID);
	glAttachShader(m_programID, m_geometryID);
	glAttachShader(m_programID, m_fragmentID);
	glProgramParameteriEXT(m_programID, GL_GEOMETRY_INPUT_TYPE_EXT, input);
	glProgramParameteriEXT(m_programID, GL_GEOMETRY_OUTPUT_TYPE_EXT, output);
	glProgramParameteriEXT(m_programID, GL_GEOMETRY_VERTICES_OUT_EXT, vertices);
	glLinkProgram(m_programID);

	printlogmsg(m_programID);
}

rynx::graphics::shader::~shader()
{
	if(m_vertexID != 0)
	{
		glDetachShader(m_programID, m_vertexID);
	}
	if(m_geometryID != 0)
	{
		glDetachShader(m_programID, m_geometryID);
	}
	if(m_fragmentID != 0)
	{
		glDetachShader(m_programID, m_fragmentID);
	}
	
	if(m_programID != 0)
	{
		glDeleteProgram(m_programID);
	}

	if(m_vertexID != 0)
	{
		glDeleteShader(m_vertexID);
	}
	if(m_geometryID != 0)
	{
		glDeleteShader(m_geometryID);
	}
	if(m_fragmentID != 0)
	{
		glDeleteShader(m_fragmentID);
	}
}

GLint rynx::graphics::shader::uniform(const std::string& name)
{
	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
	rynx_assert(m_started, "looking for uniform when shader has not been started! not good.");
	auto it = m_uniformLocations.find(name);
	if(it == m_uniformLocations.end())
	{
		GLint location = glGetUniformLocation(m_programID, name.c_str());
		m_uniformLocations.emplace(name, location);
		rynx_assert(location != -1, "shader has no uniform with name '%s'", name.c_str());
		return location;
	}
	else
	{
		return it->second;
	}
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, float value)
{
	glUniform1f(uniform(name), value);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, float value1, float value2)
{
	glUniform2f(uniform(name), value1, value2);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, float value1, float value2, float value3)
{
	glUniform3f(uniform(name), value1, value2, value3);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, float value1, float value2, float value3, float value4)
{
	glUniform4f(uniform(name), value1, value2, value3, value4);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, int32_t value)
{
	glUniform1i(uniform(name), value);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, int32_t value1, int32_t value2)
{
	glUniform2i(uniform(name), value1, value2);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3)
{
	glUniform3i(uniform(name), value1, value2, value3);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3, int32_t value4)
{
	glUniform4i(uniform(name), value1, value2, value3, value4);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, const matrix4& matrix)
{
	glUniformMatrix4fv(uniform(name), 1, false, matrix.m);
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, floats4* v, size_t amount) {
	static_assert(sizeof(floats4) == 4 * sizeof(float), "can't take array of floats4 directly because of padding.");
	glUniform4fv(uniform(name), amount, reinterpret_cast<float*>(v));
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, vec3f* v, size_t amount) {
	static_assert(sizeof(vec3f) == 3 * sizeof(float), "can't take array of vec3f directly because of padding.");
	glUniform3fv(uniform(name), amount, reinterpret_cast<float*>(v));
	return *this;
}

rynx::graphics::shader& rynx::graphics::shader::uniform(const std::string& name, float* v, size_t amount) {
	glUniform1fv(uniform(name), amount, v);
	return *this;
}


GLint rynx::graphics::shader::attribute(const std::string& name)
{
	rynx_assert(m_started, "trying to fetch attribute from a shader that is not active!");
	auto it = m_uniformLocations.find(name);
	if(it == m_uniformLocations.end())
	{
		GLint location = glGetAttribLocation(m_programID, name.c_str());
		m_uniformLocations.emplace(name, location);
		rynx_assert(location != -1, "WARNING: shader has no attribute with name '%s'", name.c_str());
		return location;
	}
	else
	{
		return it->second;
	}
}

void rynx::graphics::shader::activate()
{
	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
	rynx_assert(m_programID != 0, "trying to start invalid shader");
	if (m_started)
		return;
	
	m_started = true;
	glUseProgram(m_programID);

	if (m_shaders) {
		m_shaders->activate_shader(m_name);
	}
}

void rynx::graphics::shader::stop()
{
	m_started = false;
}
