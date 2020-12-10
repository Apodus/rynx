
#include <rynx/menu/List.hpp>
#include <rynx/input/mapped_input.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/shape.hpp>

#include <rynx/graphics/text/font.hpp>
#include <algorithm>

void rynx::menu::List::align_list_left() {
	m_scrolling_content_panel.align().left_inside();
	m_current_align = menu::Align::LEFT;
}

void rynx::menu::List::align_list_center() {
	m_scrolling_content_panel.align().center_x();
	m_current_align = menu::Align::CENTER_HORIZONTAL;
}

void rynx::menu::List::align_list_right() {
	m_scrolling_content_panel.align().right_inside();
	m_current_align = menu::Align::RIGHT;
}

void rynx::menu::List::onInput(rynx::mapped_input& input) {
	if (inRectComponent(input.mouseMenuPosition(m_aspectRatio))) {
		m_scrolling_content_panel.target_position().y += input.getMouseScroll() * 0.05f;
	}
}

void rynx::menu::List::draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const {
	m_frame.draw(meshRenderer, textRenderer);
}

void rynx::menu::List::update(float dt) {
	m_frame.tick(dt, m_aspectRatio);
	m_scrolling_content_panel.tick(dt, m_aspectRatio);

	if (!m_children.empty()) {
		float total_children_height = m_list_element_margin * (m_children.size() - 1);
		for (auto&& child : m_children) {
			total_children_height += child->scale_world().y;
		}

		float min_scroll_value = parent()->scale_world().y * 0.5f - m_list_endpoint_margin;
		float max_scroll_value = total_children_height - parent()->scale_world().y * 0.5f + m_list_endpoint_margin * 2.0f;
		if (min_scroll_value > max_scroll_value) {
			max_scroll_value = min_scroll_value;
		}

		float current_scroll_value = m_scrolling_content_panel.target_position().y;
		if (current_scroll_value > max_scroll_value) {
			m_scrolling_content_panel.target_position().y = max_scroll_value;
		}
		else if (current_scroll_value < min_scroll_value) {
			m_scrolling_content_panel.target_position().y = min_scroll_value;
		}

		if (m_current_align == menu::Align::LEFT) {
			m_children.front()->align().target(&m_scrolling_content_panel).top_inside().left_inside();
			for (size_t i = 1; i < m_children.size(); ++i) {
				m_children[i]->align()
					.target(m_children[i - 1].get())
					.offset_kind_relative_to_parent()
					.offset_y(m_list_element_margin)
					.bottom_outside()
					.left_inside();
			}
		}
		else if (m_current_align == menu::Align::RIGHT) {
			m_children.front()->align().target(&m_scrolling_content_panel).top_inside().right_inside();
			for (size_t i = 1; i < m_children.size(); ++i) {
				m_children[i]->align()
					.target(m_children[i - 1].get())
					.offset_kind_relative_to_parent()
					.offset_y(m_list_element_margin)
					.bottom_outside()
					.right_inside();
			}
		}
		else {
			m_children.front()->align().target(&m_scrolling_content_panel).top_inside().center_x();
			for (size_t i = 1; i < m_children.size(); ++i) {
				m_children[i]->align()
					.target(m_children[i - 1].get())
					.offset_kind_relative_to_parent()
					.offset_y(m_list_element_margin)
					.bottom_outside()
					.center_x();
			}
		}

		float container_top_edge = position_world().y + scale_world().y * 0.5f - m_list_endpoint_margin;
		float container_bot_edge = position_world().y - scale_world().y * 0.5f + m_list_endpoint_margin;
		
		for (auto&& child : m_children) {
			child->velocity_position(m_list_elements_velocity);
			float dst_alpha = 1.0f;

			float child_top_edge = child->position_world().y + child->scale_world().y * 0.33f;
			float child_bot_edge = child->position_world().y - child->scale_world().y * 0.33f;

			if (child_top_edge > container_top_edge) {
				float outside = (child_top_edge - container_top_edge) / (m_list_endpoint_margin + 0.01f);
				outside *= outside;
				outside *= outside;
				dst_alpha = std::clamp(1.0f - outside, 0.0f, 1.0f);
				child->color().w *= dst_alpha;
			}
			if (child_bot_edge < container_bot_edge) {
				float outside = (container_bot_edge - child_bot_edge) / (m_list_endpoint_margin + 0.01f);
				outside *= outside;
				outside *= outside;
				dst_alpha = std::clamp(1.0f - outside, 0.0f, 1.0f);
				child->color().w *= dst_alpha;
			}
		}
	}
}