
#pragma once

#include <rynx/menu/Frame.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Component.hpp>

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

#include <functional>

class Font;

namespace rynx {
	class mapped_input;

	namespace menu {

		class List : public Component {
		protected:
			

			Frame m_frame;
			Font* m_font = nullptr;
			Div m_scrolling_content_panel;

			rynx::menu::Align m_current_align = rynx::menu::Align::LEFT;
			float m_list_endpoint_margin = 0.0f;
			float m_list_element_margin = 0.0f;
			float m_list_elements_velocity = 5.0f;

		public:
			List(
				rynx::graphics::texture_id texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>(),
				float frame_edge_percentage = 0.2f
			) : Component(scale, position)
				, m_frame(texture, frame_edge_percentage)
				, m_scrolling_content_panel({0, 0, 0})
			{
				m_frame.set_parent(this);
				m_scrolling_content_panel.set_parent(this);
				m_scrolling_content_panel.align().left_inside();
			}

			void align_list_left();
			void align_list_center();
			void align_list_right();

			void list_endpoint_margin(float margin) { m_list_endpoint_margin = margin; }
			float list_endpoint_margin() const { return m_list_endpoint_margin; }

			void list_element_margin(float margin) { m_list_element_margin = margin; }
			float list_element_margin() const { return m_list_element_margin; }


			void list_element_velocity(float velocity) {
				m_list_elements_velocity = velocity;
				m_scrolling_content_panel.velocity_position(velocity);
			}
			float list_endpoint_velocity() const { return m_list_elements_velocity; }


		private:
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
		};
	}
}
