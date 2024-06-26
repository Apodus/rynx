
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/std/string.hpp>

typedef int GLint;
typedef unsigned GLuint;


namespace rynx {
	class matrix4;
	namespace graphics {
		class shaders;
		class GraphicsDLL shader
		{
		public:
			shader(rynx::string name, const char* vertexSource, const char* fragmentSource);
			shader(rynx::string name, const char* vertexSource, const char* fragmentSource, const char* geometrySource, GLint input, GLint output, GLint vertices);
			~shader();

			void activate();
			void stop();

			GLint uniform(const rynx::string& name);

			shader& uniform(const rynx::string& name, float value);
			shader& uniform(const rynx::string& name, float value1, float value2);
			shader& uniform(const rynx::string& name, float value1, float value2, float value3);
			shader& uniform(const rynx::string& name, float value1, float value2, float value3, float value4);

			shader& uniform(const rynx::string& name, vec3f v) { return uniform(name, v.x, v.y, v.z); }
			shader& uniform(const rynx::string& name, floats4 v) { return uniform(name, v.x, v.y, v.z, v.w); }
			
			shader& uniform(const rynx::string& name, floats4* v, size_t amount);
			shader& uniform(const rynx::string& name, vec3f* v, size_t amount);
			shader& uniform(const rynx::string& name, float* v, size_t amount);

			shader& uniform(const rynx::string& name, int32_t value);
			shader& uniform(const rynx::string& name, int32_t value1, int32_t value2);
			shader& uniform(const rynx::string& name, int32_t value1, int32_t value2, int32_t value3);
			shader& uniform(const rynx::string& name, int32_t value1, int32_t value2, int32_t value3, int32_t value4);
			shader& uniform(const rynx::string& name, const rynx::matrix4& mat);

			GLint attribute(const rynx::string& name);
			// TODO: Friendly attribute setting api.

			void set_shader_manager(shaders* shaders) { m_shaders = shaders; }

		private:
			shader() = delete;
			shader(const shader& shader) = delete;
			shader& operator=(const shader& shader) = delete;

			rynx::string m_name;
			shaders* m_shaders = nullptr;

			GLuint m_programID;
			GLuint m_vertexID;
			GLuint m_geometryID;
			GLuint m_fragmentID;

			rynx::unordered_map<rynx::string, GLint> m_uniformLocations;
			bool m_started;
		};
	}
}