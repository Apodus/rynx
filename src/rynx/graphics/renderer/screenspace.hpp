#pragma once

namespace rynx {
	namespace graphics {
		class screenspace_draws {
		public:
			screenspace_draws();
			static void draw_fullscreen();
			static void clear_screen();
		};
	}
}