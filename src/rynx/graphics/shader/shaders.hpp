
#pragma once

#include <rynx/graphics/shader/shader.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/std/string.hpp>

typedef int GLint;
typedef unsigned GLuint;

namespace rynx {
	namespace graphics {
		class GraphicsDLL shaders
		{
		public:
			shaders();
			~shaders();

			void release();

			rynx::shared_ptr<shader> load_shader(const rynx::string& name, const rynx::string& vertexShader, const rynx::string& fragmentShader);
			rynx::shared_ptr<shader> activate_shader(const rynx::string& name);

		private:
			rynx::unordered_map<rynx::string, rynx::shared_ptr<shader>> m_shaders;

			rynx::shared_ptr<shader> m_activeShader;
			rynx::string m_activeShaderName;
		};
	}
}