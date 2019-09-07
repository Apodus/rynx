
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/shader/shader.hpp>
#include <rynx/system/assert.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace std;

std::string Shader::readFile(const std::string& path)
{
	std::stringstream ss;
	std::ifstream file(path);
	ss << file.rdbuf();
	file.close();
	rynx_assert(ss.str().length() > 0, "Shader source code length zero!");

	return ss.str();
}

GLuint Shader::loadVertexShader(const std::string& filename)
{
	std::string str = readFile(filename);
	const char* data = str.c_str();
	
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(ret, 1, &data, nullptr);
	glCompileShader(ret);

	return ret;
}

GLuint Shader::loadFragmentShader(const std::string& filename)
{
	std::string str = readFile(filename);
	const char* data = str.c_str();

	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ret, 1, &data, nullptr);
	glCompileShader(ret);

	return ret;
}

GLuint Shader::loadGeometryShader(const string& filename)
{
	std::string str = readFile(filename);
	const char* data = str.c_str();

	GLuint ret = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(ret, 1, &data, nullptr);
	glCompileShader(ret);

	return ret;
}

void Shader::printlogmsg(GLuint obj)
{
	int infologLength = 0;
	char infoLog[1024];
	
	if (glIsShader(obj))
		glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);
	
	if (infoLog[0])
	{
		printf("Shader build info: %s", infoLog);
	}
}


Shader::Shader(const std::string& vert, const std::string& frag):
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
		logmsg("gl error value: %d", errorValue);
	}
	printlogmsg(m_programID);
}

Shader::Shader(const std::string& vert, const std::string& frag, const std::string& geom, GLint input, GLint output, GLint vertices):
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

Shader::~Shader()
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

void Shader::set_texture_unit(size_t unit, const std::string& name)
{
	rynx_assert(m_started, "can't set texture unit until program has been set active");
	GLint texture_unit_location = uniform(name);
	rynx_assert(texture_unit_location != -1, "WARNING: shader has no sampler with name '%s'", name.c_str());
	glUniform1i(texture_unit_location, static_cast<GLint>(unit));
}

GLint Shader::uniform(const std::string& name)
{
	rynx_assert(m_started, "looking for uniform when shader has not been started! not good.");
	auto it = m_uniformLocations.find(name);
	if(it == m_uniformLocations.end())
	{
		GLint location = glGetUniformLocation(m_programID, name.c_str());
		m_uniformLocations.insert(it, make_pair(name, location));
		rynx_assert(location != -1, "shader has no uniform with name '%s'", name.c_str());
		return location;
	}
	else
	{
		return it->second;
	}
}

GLint Shader::attribute(const std::string& name)
{
	rynx_assert(m_started, "trying to fetch attribute from a shader that is not active!");
	auto it = m_uniformLocations.find(name);
	if(it == m_uniformLocations.end())
	{
		GLint location = glGetAttribLocation(m_programID, name.c_str());
		m_uniformLocations.insert(it, make_pair(name, location));
		rynx_assert(location != -1, "WARNING: shader has no attribute with name '%s'", name.c_str());
		return location;
	}
	else
	{
		return it->second;
	}
}

void Shader::start()
{
	rynx_assert(m_programID != 0, "trying to start invalid shader");
	rynx_assert(!m_started, "shader already started!");
	m_started = true;
	glUseProgram(m_programID);
}

void Shader::stop()
{
	rynx_assert(m_started, "stopping shader that has not been started");
	glUseProgram(0);
	m_started = false;
}

GLuint Shader::get_program() const
{
	return m_programID;
}






#include <rynx/tech/math/matrix.hpp>

// it is bad form to ask for positions every time again and again.

void ShaderMemory::setUniformVec1(Shader& shader, const std::string& name, float value) {
	int pos = shader.uniform(name);
    glUniform1f(pos, value);
}

void ShaderMemory::setUniformVec2(Shader& shader, const std::string& name, float value1, float value2) {
	int pos = shader.uniform(name);
    glUniform2f(pos, value1, value2);
}

void ShaderMemory::setUniformVec3(Shader& shader, const std::string& name, float value1, float value2, float value3) {
    int pos = shader.uniform(name);
    glUniform3f(pos, value1, value2, value3);
}

void ShaderMemory::setUniformVec4(Shader& shader, const std::string& name, float value1, float value2, float value3, float value4) {
    int pos = shader.uniform(name);
    glUniform4f(pos, value1, value2, value3, value4);
}

void ShaderMemory::setUniformMat4(Shader& shader, const std::string& name, const matrix4& matrix) {
	int pos = shader.uniform(name);
    glUniformMatrix4fv(pos, 1, false, matrix.data);
}

