#pragma once

namespace rynx {
	namespace graphics {
		class GraphicsDLL screenspace_draws {
		public:
			screenspace_draws();
			static void draw_fullscreen();
			static void clear_screen();
		};
	}
}