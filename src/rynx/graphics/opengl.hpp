
#pragma once

namespace rynx {
	namespace graphics {
		namespace gl {
			GraphicsDLL void enable_blend();
			GraphicsDLL void disable_blend();

			GraphicsDLL void enable_blend_for(int render_target);
			GraphicsDLL void disable_blend_for(int render_target);

			GraphicsDLL void blend_func_cumulative();
			GraphicsDLL void blend_func_cumulative_for(int render_target);

			GraphicsDLL void blend_func_default();
			GraphicsDLL void blend_func_default_for(int render_target);
		}
	}
}