
#pragma once

#include <rynx/graphics/shader/shader.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <string>
#include <memory>

typedef int GLint;
typedef unsigned GLuint;

namespace rynx {
	namespace graphics {
		class shaders
		{
		public:
			shaders();
			~shaders();

			void release();

			std::shared_ptr<shader> load_shader(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader);
			std::shared_ptr<shader> activate_shader(const std::string& name);

		private:
			rynx::unordered_map<std::string, std::shared_ptr<shader>> m_shaders;

			std::shared_ptr<shader> m_activeShader;
			std::string m_activeShaderName;
		};
	}
}