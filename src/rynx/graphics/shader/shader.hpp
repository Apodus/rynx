
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <string>

typedef int GLint;
typedef unsigned GLuint;

class matrix4;

namespace rynx {
	namespace graphics {
		class Shaders;
		class Shader
		{
		public:
			Shader(std::string name, const std::string& vertex, const std::string& fragment);
			Shader(std::string name, const std::string& vertex, const std::string& fragment, const std::string& geometry, GLint input, GLint output, GLint vertices);
			~Shader();

			void activate();
			void stop();

			GLint uniform(const std::string& name);

			Shader& uniform(const std::string& name, float value);
			Shader& uniform(const std::string& name, float value1, float value2);
			Shader& uniform(const std::string& name, float value1, float value2, float value3);
			Shader& uniform(const std::string& name, float value1, float value2, float value3, float value4);

			Shader& uniform(const std::string& name, vec3f v) { return uniform(name, v.x, v.y, v.z); }
			Shader& uniform(const std::string& name, floats4 v) { return uniform(name, v.x, v.y, v.z, v.w); }

			Shader& uniform(const std::string& name, int32_t value);
			Shader& uniform(const std::string& name, int32_t value1, int32_t value2);
			Shader& uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3);
			Shader& uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3, int32_t value4);
			Shader& uniform(const std::string& name, const matrix4& mat);

			GLint attribute(const std::string& name);
			// TODO: Friendly attribute setting api.

			void set_shader_manager(Shaders* shaders) { m_shaders = shaders; }

		private:
			Shader() = delete;
			Shader(const Shader& shader) = delete;
			Shader& operator=(const Shader& shader) = delete;

			std::string m_name;
			Shaders* m_shaders = nullptr;

			GLuint m_programID;
			GLuint m_vertexID;
			GLuint m_geometryID;
			GLuint m_fragmentID;

			rynx::unordered_map<std::string, GLint> m_uniformLocations;
			bool m_started;
		};
	}
}