
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <string>

typedef int GLint;
typedef unsigned GLuint;


namespace rynx {
	class matrix4;
	namespace graphics {
		class shaders;
		class shader
		{
		public:
			shader(std::string name, const std::string& vertex, const std::string& fragment);
			shader(std::string name, const std::string& vertex, const std::string& fragment, const std::string& geometry, GLint input, GLint output, GLint vertices);
			~shader();

			void activate();
			void stop();

			GLint uniform(const std::string& name);

			shader& uniform(const std::string& name, float value);
			shader& uniform(const std::string& name, float value1, float value2);
			shader& uniform(const std::string& name, float value1, float value2, float value3);
			shader& uniform(const std::string& name, float value1, float value2, float value3, float value4);

			shader& uniform(const std::string& name, vec3f v) { return uniform(name, v.x, v.y, v.z); }
			shader& uniform(const std::string& name, floats4 v) { return uniform(name, v.x, v.y, v.z, v.w); }
			
			shader& uniform(const std::string& name, floats4* v, size_t amount);
			shader& uniform(const std::string& name, vec3f* v, size_t amount);
			shader& uniform(const std::string& name, float* v, size_t amount);

			shader& uniform(const std::string& name, int32_t value);
			shader& uniform(const std::string& name, int32_t value1, int32_t value2);
			shader& uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3);
			shader& uniform(const std::string& name, int32_t value1, int32_t value2, int32_t value3, int32_t value4);
			shader& uniform(const std::string& name, const rynx::matrix4& mat);

			GLint attribute(const std::string& name);
			// TODO: Friendly attribute setting api.

			void set_shader_manager(shaders* shaders) { m_shaders = shaders; }

		private:
			shader() = delete;
			shader(const shader& shader) = delete;
			shader& operator=(const shader& shader) = delete;

			std::string m_name;
			shaders* m_shaders = nullptr;

			GLuint m_programID;
			GLuint m_vertexID;
			GLuint m_geometryID;
			GLuint m_fragmentID;

			rynx::unordered_map<std::string, GLint> m_uniformLocations;
			bool m_started;
		};
	}
}