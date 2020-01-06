#pragma once

#include <cstdint>
#include <memory>


namespace rynx {
	namespace graphics {
		class shaders;
		class screenspace_draws {
		public:
			screenspace_draws(std::shared_ptr<shaders> shaders);
			void draw_fullscreen();
			
			// TODO: this is probably not the right place for these.
			void blend_mode_cumulative();
			void blend_mode_default();
		private:
			std::shared_ptr<shaders> m_shaders;
			uint32_t vao;
			uint32_t vbo;
		};
	}
}