#pragma once

#include <cstdint>
#include <memory>


namespace rynx {
	namespace graphics {
		class screenspace_draws {
		public:
			screenspace_draws();
			static void draw_fullscreen();
			static void clear_screen();

			// TODO: this is probably not the right place for these.
			static void blend_mode_cumulative();
			static void blend_mode_default();
		};
	}
}