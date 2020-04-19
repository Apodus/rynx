
#pragma once

namespace rynx {
	namespace graphics {
		namespace gl {
			void enable_blend();
			void disable_blend();

			void enable_blend_for(int render_target);
			void disable_blend_for(int render_target);

			void blend_func_cumulative();
			void blend_func_cumulative_for(int render_target);

			void blend_func_default();
			void blend_func_default_for(int render_target);
		}
	}
}