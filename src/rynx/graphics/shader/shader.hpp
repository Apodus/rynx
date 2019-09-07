
#pragma once

#include <string>
#include <map>

typedef int GLint;
typedef unsigned GLuint;

class Shaders;

class Shader
{
	Shader(const std::string& vertex, const std::string& fragment);
	Shader(const std::string& vertex, const std::string& fragment, const std::string& geometry, GLint input, GLint output, GLint vertices);

	void start();
	void stop();

	friend class Shaders;

public:
	~Shader();

	void set_texture_unit(size_t unit, const std::string& name);
	GLint uniform(const std::string& name);
	GLint attribute(const std::string& name);
	GLuint get_program() const;

private:
	Shader();
	Shader(const Shader& shader);
	Shader& operator=(const Shader& shader); // disabled for all of these

	static std::string readFile(const std::string&);
	static void printlogmsg(GLuint obj);
	static GLuint loadVertexShader(const std::string& filename);
	static GLuint loadFragmentShader(const std::string& filename);
	static GLuint loadGeometryShader(const std::string& filename);

	GLuint m_programID;
	GLuint m_vertexID;
	GLuint m_geometryID;
	GLuint m_fragmentID;
	
	std::map<std::string, GLint> m_uniformLocations;
	bool m_started;
};

class matrix4;

namespace ShaderMemory {
	void setUniformVec1(Shader& shader, const std::string& name, float value);
	void setUniformVec2(Shader& shader, const std::string& name, float value1, float value2);
	void setUniformVec3(Shader& shader, const std::string& name, float value1, float value2, float value3);
	void setUniformVec4(Shader& shader, const std::string& name, float value1, float value2, float value3, float value4);
	void setUniformMat4(Shader& shader, const std::string& name, const matrix4& matrix);
}