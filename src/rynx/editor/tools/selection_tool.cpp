
#include <rynx/editor/tools/selection_tool.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/math/geometry/bounding_sphere.hpp>
#include <rynx/math/geometry/sphere.hpp>
#include <rynx/system/typeid.hpp>

#include <rynx/menu/Div.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/Text.hpp>

rynx::editor::tools::selection_tool::selection_tool(rynx::scheduler::context& ctx) {
	auto& input = ctx.get_resource<rynx::mapped_input>();
	m_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(0), "selection tool activate");

	define_action(rynx::traits::type_name<rynx::id>(), "pick entity", [this](rynx::scheduler::context* ctx) {
		m_mode = Mode::IdField_Pick;
	});
}

bool rynx::editor::tools::selection_tool::operates_on(const std::string& type_name) {
	return rynx::traits::type_name<rynx::id>() == type_name;
}

bool rynx::editor::tools::selection_tool::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	rynx::reflection::type type = info.reflections->get(field_type);
	if (type.m_type_name == rynx::traits::type_name<rynx::id>())
	{
		auto field_div = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		auto id_pick_button = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(1.0f, 1.0f, 0.0f));
		id_pick_button->velocity_position(200.0f); // TODO
		id_pick_button->on_click([this, info, self = id_pick_button.get()]() {
			this->source_data([this, info]() {
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::id*>(data + info.cumulative_offset);
				return value;
			});
			this->m_mode = Mode::IdField_Pick;
			this->source_data_cb([](void* data) {
				// TODO: might as well update the id info to the UI here
			});
		});

		id_pick_button->text().text(field_type.m_field_name + ": Pick entity");

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		field_div->addChild(id_pick_button);
		info.component_sheet->addChild(field_div);
		return true;
	}

	if (type.m_type_name == rynx::traits::type_name<rynx::vec3f>())
	{
		{
			auto field_div = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
			field_div->velocity_position(200.0f); // TODO
			field_div->text().text(field_type.m_field_name);
			field_div->align()
				.target(info.component_sheet->last_child())
				.bottom_outside()
				.left_inside();
			info.component_sheet->addChild(field_div);
		}

		auto field_div = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
		auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);

		std::vector<std::function<void(void* data)>> update_ui;
		{
			auto x_text = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			x_text->text("x");
			x_text->align().left_inside().top_inside();
			x_text->velocity_position(200.0f);

			auto x_value = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			x_value->velocity_position(200.0f); // TODO
			x_value->text().text(std::to_string(value->x));
			x_value->text().text_input_enable();
			x_value->align().target(x_text.get()).right_outside().top_inside();
			x_value->text().on_value_changed([info, self = x_value.get()](std::string s) {
				float new_v = std::stof(s);
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->x = new_v;
				self->text().text(std::to_string(value->x));
			});

			update_ui.emplace_back([ui = x_value.get()](void* ptr) {
				ui->text().text(std::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->x));
			});

			field_div->addChild(x_text);
			field_div->addChild(x_value);
		}

		{
			auto y_text = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			y_text->text("y");
			y_text->align().target(field_div->last_child()).right_outside().top_inside();
			y_text->velocity_position(200.0f);

			auto y_value = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			y_value->velocity_position(200.0f); // TODO
			y_value->text().text(std::to_string(value->y));
			y_value->text().text_input_enable();
			y_value->align().target(y_text.get()).right_outside().top_inside();
			y_value->text().on_value_changed([info, self = y_value.get()](std::string s) {
				float new_v = std::stof(s);
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->y = new_v;
				self->text().text(std::to_string(value->y));
			});
			
			update_ui.emplace_back([ui = y_value.get()](void* ptr) {
				ui->text().text(std::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->y));
			});
			
			field_div->addChild(y_text);
			field_div->addChild(y_value);
		}

		{
			auto z_text = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			z_text->text("z");
			z_text->align().target(field_div->last_child()).right_outside().top_inside();
			z_text->velocity_position(200.0f);

			auto z_value = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			z_value->velocity_position(200.0f); // TODO
			z_value->text().text(std::to_string(value->z));
			z_value->text().text_input_enable();
			z_value->align().target(z_text.get()).right_outside().top_inside();
			z_value->text().on_value_changed([info, self = z_value.get()](std::string s) {
				float new_v = std::stof(s);
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->z = new_v;
				self->text().text(std::to_string(value->z));
			});

			update_ui.emplace_back([ui = z_value.get()](void* ptr) {
				ui->text().text(std::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->z));
			});

			field_div->addChild(z_text);
			field_div->addChild(z_value);
		}

		{
			auto drag = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.10f, 0.8f, 0.0f));
			drag->velocity_position(200.0f); // TODO
			drag->text().text("drag");
			drag->text().color({0.5f, 1.0f, 0.0f, 1.0f});
			drag->align().target(field_div->last_child()).right_outside().top_inside();
			drag->on_click([this, info, update_ui]() {
				source_data([info]() {
					char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
					auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
					return value;
				});
				source_data_cb([this, update_ui](void* data) {
					for (auto&& op : update_ui) {
						op(data);
					}
				});
				m_mode = Mode::Vec3fDrag;
			});

			field_div->addChild(drag);
		}

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		info.component_sheet->addChild(field_div);
		return true;
	}

	return false;
}


void rynx::editor::tools::selection_tool::update(rynx::scheduler::context& ctx) {
	if (m_run_on_main_thread) {
		m_run_on_main_thread();
		m_run_on_main_thread = nullptr;
	}

	ctx.add_task("editor tick", [this, &ctx](
		rynx::ecs& game_ecs,
		rynx::mapped_input& gameInput,
		rynx::camera& gameCamera)
		{
			auto mouseRay = gameInput.mouseRay(gameCamera);
			auto [mouse_z_plane, hit] = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
			mouse_z_plane.z = 0;

			if (gameInput.isKeyClicked(m_activation_key) && !gameInput.isKeyConsumed(m_activation_key)) {
				
				if (hit) {
					auto [id, distance] = find_nearest_entity(game_ecs, mouse_z_plane);

					if (m_mode == Mode::IdField_Pick) {
						if (id != 0) {
							auto* data = reinterpret_cast<rynx::id*>(address_of_operand());
							*data = id;
							operand_value_changed(data);
							m_mode = Mode::Default;
							return;
						}
					}
					else if (m_mode == Mode::Default) {
						// select new selection
						if (gameInput.isKeyDown(rynx::key::codes::control_left()) || gameInput.isKeyDown(rynx::key::codes::control_right()))
						{
							auto remove_if_selected = [this](rynx::id id) {
								for (auto it = m_editor_state->m_selected_ids.begin(); it != m_editor_state->m_selected_ids.end(); ++it) {
									if (*it == id) {
										*it = m_editor_state->m_selected_ids.back();
										m_editor_state->m_selected_ids.pop_back();
										return true;
									}
								}
								return false;
							};

							if (remove_if_selected(id)) {
								// was removed
							}
							else {
								// need to select
								m_editor_state->m_selected_ids.emplace_back(id);
							}
						}
						else {
							if (m_editor_state->m_on_entity_selected) {
								m_run_on_main_thread = [this, selected_entity = id]() {
									m_editor_state->m_on_entity_selected(selected_entity);
								};
							}
						}
					}
				}
			}

			// drag start
			if (gameInput.isKeyPressed(m_activation_key)) {
				if (hit) {
					m_drag.prev_point = mouse_z_plane;
					m_drag.start_point = mouse_z_plane;

					// calculate some middle point
					{
						const auto& ids = selected_id_vector();
						if (!ids.empty()) {
							std::vector<rynx::sphere> bounds;
							for (auto id : ids) {
								auto entity = game_ecs[id];
								auto& entity_pos = entity.get<rynx::components::position>();
								auto& entity_radius = entity.get<rynx::components::radius>();
								bounds.emplace_back(entity_pos.value, entity_radius.r);
							}

							auto [point, radius] = rynx::math::bounding_sphere(bounds);
							m_drag.middle_point = point;

							auto mouse_total_delta_middle = mouse_z_plane - m_drag.middle_point;
							m_drag.previous_angle = std::atan2(mouse_total_delta_middle.y, mouse_total_delta_middle.x);
						}
					}
				}
			}

			// drag update
			if (gameInput.isKeyDown(m_activation_key)) {
				if ((m_drag.prev_point - mouse_z_plane).length_squared() > 1.0f) {
					m_drag.active = true;
				}
				
				if (m_drag.active) {
					auto mouse_total_delta = mouse_z_plane - m_drag.start_point;
					auto mouse_total_delta_middle = mouse_z_plane - m_drag.middle_point;
					rynx::vec3f position_delta = mouse_z_plane - m_drag.prev_point;
					float current_angle = std::atan2(mouse_total_delta_middle.y, mouse_total_delta_middle.x);
					float delta_angle = current_angle - m_drag.previous_angle;
					
					m_drag.prev_point = mouse_z_plane;
					m_drag.previous_angle = current_angle;

					if (m_mode == Mode::Default) {
						if (gameInput.isKeyDown(rynx::key::codes::alt_left()) || gameInput.isKeyDown(rynx::key::codes::alt_right())) {
							// drag entity rotation
							const auto& ids = selected_id_vector();

							for (auto id : ids) {
								auto entity = game_ecs[id];
								auto& entity_pos = entity.get<rynx::components::position>();

								entity_pos.value = rynx::math::rotatedXY((entity_pos.value - m_drag.middle_point), delta_angle) + m_drag.middle_point;
								entity_pos.angle += delta_angle;

								execute([this, &game_ecs, &ctx, id]() {
									for_each_tool([id, &game_ecs, &ctx](rynx::editor::itool* tool) {
										tool->on_entity_component_value_changed(&ctx, "rynx::components::position", game_ecs, id);
										});
									});
							}
						}
						else {
							// drag entity position
							const auto& ids = selected_id_vector();
							for (auto id : ids) {
								auto entity = game_ecs[id];
								auto& entity_pos = entity.get<rynx::components::position>();
								entity_pos.value += position_delta;

								execute([this, &game_ecs, &ctx, id]() {
									for_each_tool([id, &game_ecs, &ctx](rynx::editor::itool* tool) {
										tool->on_entity_component_value_changed(&ctx, "rynx::components::position", game_ecs, id);
										});
									});
							}
						}
					}
					else if (m_mode == Mode::Vec3fDrag) {
						// TODO: Some callback update mechanism back to the data?
						auto* data = reinterpret_cast<rynx::vec3f*>(address_of_operand());
						*data += position_delta;
						operand_value_changed(data);
					}
				}
			}

			// drag end
			if (gameInput.isKeyReleased(m_activation_key)) {
				if (m_mode == Mode::Vec3fDrag) {
					m_mode = Mode::Default;
				}
				if (m_drag.active) {
					m_drag.active = false;
				}
			}
		}
	);
}

std::pair<rynx::ecs::id, float> rynx::editor::tools::selection_tool::find_nearest_entity(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	float best_distance = 1e30f;
	rynx::ecs::id best_id;

	// find best selection
	game_ecs.query().for_each([&, mouse_world_pos = cursorWorldPos](rynx::ecs::id id, rynx::components::position pos) {
		float sqr_dist = (mouse_world_pos - pos.value).length_squared();
		auto* boundary_ptr = game_ecs[id].try_get<rynx::components::boundary>();
		auto* radius_ptr = game_ecs[id].try_get<rynx::components::radius>();

		if (sqr_dist < best_distance) {
			best_distance = sqr_dist;
			best_id = id;
		}

		if (boundary_ptr) {
			for (size_t i = 0; i < boundary_ptr->segments_world.size(); ++i) {
				// some threshold for vertex picking
				const auto vertex = boundary_ptr->segments_world.segment(i);
				auto [dist, shortest_pos] = rynx::math::pointDistanceLineSegmentSquared(vertex.p1, vertex.p2, mouse_world_pos);
				if (dist < best_distance) {
					best_distance = dist;
					best_id = id;
				}
			}
		}
		else if (radius_ptr) {
			float some_distance = (mouse_world_pos - pos.value).length_squared() - radius_ptr->r * radius_ptr->r;
			if (some_distance < 0)
				some_distance /= radius_ptr->r * radius_ptr->r;

			if (some_distance < best_distance) {
				best_distance = sqr_dist;
				best_id = id;
			}
		}
	});
	
	return {best_id, best_distance};
}
