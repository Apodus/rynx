#pragma once

#include <cstdint>
#include <memory>

class Shaders;

namespace rynx {
	namespace graphics {
		class screenspace_renderer {
		public:
			screenspace_renderer(std::shared_ptr<Shaders> shaders);
			void draw_fullscreen();

		private:
			std::shared_ptr<Shaders> m_shaders;
			uint32_t vao;
			uint32_t vbo;
		};
	}
}