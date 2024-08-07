
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

	define_action(rynx::type_index::id<rynx::id>(), "pick entity", [this](rynx::scheduler::context*) {
		m_mode = Mode::IdField_Pick;
	});
}

bool rynx::editor::tools::selection_tool::operates_on(rynx::type_id_t type_id) {
	return type_id == rynx::type_index::id<rynx::id>();
}

bool rynx::editor::tools::selection_tool::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	auto create_vec3f_menu = [this](
		rynx::string field_name,
		rynx::editor::component_recursion_info_t info,
		rynx::function<rynx::id()> entity_relative_space)
	{
		rynx::id drag_in_local_space_of = entity_relative_space();
		auto field_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
		auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);

		std::vector<rynx::function<void(void* data)>> update_ui;

		{
			auto field_name_label = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.15f, 1.0f, 0.0f));
			field_name_label->velocity_position(200.0f); // TODO
			field_name_label->text(field_name);
			field_name_label->text_align_left();
			field_name_label->align()
				.left_inside()
				.top_inside()
				.offset_x(-0.2f);
			field_div->addChild(field_name_label);
		}

		{
			auto x_text = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			x_text->text("x");
			x_text->align().target(field_div->last_child()).right_outside().top_inside();
			x_text->velocity_position(200.0f);

			auto x_value = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			x_value->velocity_position(200.0f); // TODO
			x_value->text().text(rynx::to_string(value->x));
			x_value->text().text_input_enable();
			x_value->align().target(x_text.get()).right_outside().top_inside();
			x_value->text().on_value_changed([info, self = x_value.get()](rynx::string s) {
				float new_v = rynx::stof(s);
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->x = new_v;
				self->text().text(rynx::to_string(value->x));
			});

			x_value->on_update([info, self = x_value.get()]() {
				if (!self->text().has_dedicated_keyboard_input()) {
					if (info.ecs->exists(info.entity_id)) {
						char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
						auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
						self->text().text(rynx::to_string(value->x));
					}
				}
			});

			update_ui.emplace_back([ui = x_value.get()](void* ptr) {
				ui->text().text(rynx::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->x));
			});

			field_div->addChild(x_text);
			field_div->addChild(x_value);
		}

		{
			auto y_text = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			y_text->text("y");
			y_text->align().target(field_div->last_child()).right_outside().top_inside();
			y_text->velocity_position(200.0f);

			auto y_value = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			y_value->velocity_position(200.0f); // TODO
			y_value->text().text(rynx::to_string(value->y));
			y_value->text().text_input_enable();
			y_value->align().target(y_text.get()).right_outside().top_inside();
			y_value->text().on_value_changed([info, self = y_value.get()](rynx::string s) {
				float new_v = rynx::stof(s.c_str());
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->y = new_v;
				self->text().text(rynx::to_string(value->y));
			});

			y_value->on_update([info, self = y_value.get()]() {
				if (!self->text().has_dedicated_keyboard_input()) {
					if (info.ecs->exists(info.entity_id)) {
						char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
						auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
						self->text().text(rynx::to_string(value->y));
					}
				}
			});

			update_ui.emplace_back([ui = y_value.get()](void* ptr) {
				ui->text().text(rynx::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->y));
			});

			field_div->addChild(y_text);
			field_div->addChild(y_value);
		}

		{
			auto z_text = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.04f, 1.0f, 0.0f));
			z_text->text("z");
			z_text->align().target(field_div->last_child()).right_outside().top_inside();
			z_text->velocity_position(200.0f);

			auto z_value = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.8f, 0.0f));
			z_value->velocity_position(200.0f); // TODO
			z_value->text().text(rynx::to_string(value->z));
			z_value->text().text_input_enable();
			z_value->align().target(z_text.get()).right_outside().top_inside();
			z_value->text().on_value_changed([info, self = z_value.get()](rynx::string s) {
				float new_v = rynx::stof(s.c_str());
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
				value->z = new_v;
				self->text().text(rynx::to_string(value->z));
			});

			z_value->on_update([info, self = z_value.get()]() {
				if (!self->text().has_dedicated_keyboard_input()) {
					if (info.ecs->exists(info.entity_id)) {
						char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
						auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
						self->text().text(rynx::to_string(value->z));
					}
				}
			});

			update_ui.emplace_back([ui = z_value.get()](void* ptr) {
				ui->text().text(rynx::to_string(reinterpret_cast<rynx::vec3f*>(ptr)->z));
			});

			field_div->addChild(z_text);
			field_div->addChild(z_value);
		}

		{
			auto drag = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.10f, 0.8f, 0.0f));
			drag->velocity_position(200.0f); // TODO
			drag->text().text("drag");
			drag->text().color({ 0.5f, 1.0f, 0.0f, 1.0f });
			drag->align().target(field_div->last_child()).right_outside().top_inside();
			drag->on_click([this, info, drag_in_local_space_of, update_ui]() {
				source_data([info]() {
					char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
					auto* value = reinterpret_cast<rynx::vec3f*>(data + info.cumulative_offset);
					return value;
					});
				source_data_cb([this, info, update_ui](void* data) {
					for (auto&& op : update_ui) {
						op(data);
					}
					auto* type_ptr = info.reflections->find(info.component_type_id);
					if (type_ptr) {
						this->for_each_tool([this, type_ptr, info](rynx::editor::itool* tool) {
							tool->on_entity_component_value_changed(
								m_editor_state->m_editor->get_context(),
								type_ptr->m_type_index_value,
								*info.ecs,
								info.entity_id
							);
						});
					}
					});
				m_mode = Mode::Vec3fDrag;
				m_mode_local_space_id = drag_in_local_space_of;
			});

			field_div->addChild(drag);
		}

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		info.component_sheet->addChild(field_div);
	};

	rynx::reflection::type type = info.reflections->get(field_type);
	if (type.m_type_name == rynx::traits::type_name<rynx::components::transform::position_relative>()) {
		for (auto field : type.m_fields) {
			if (field.m_field_name == "id") {
				info.cumulative_offset += field.m_memory_offset;
				try_generate_menu(field, info, reflection_stack);
				info.cumulative_offset -= field.m_memory_offset;
			}
			if (field.m_field_name == "pos") {
				info.cumulative_offset += field.m_memory_offset;
				create_vec3f_menu("offset", info, [info]() {
					auto local_position =
						rynx::editor::ecs_value_editor().access<rynx::components::transform::position_relative>(
							*info.ecs,
							info.entity_id,
							info.component_type_id,
							info.cumulative_offset);
					return local_position.id;
					});
				info.cumulative_offset -= field.m_memory_offset;
			}
		}
		return true;
	}
	
	if (type.m_type_name == rynx::traits::type_name<rynx::id>())
	{
		auto field_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		auto id_pick_button = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.7f, 1.0f, 0.0f));
		id_pick_button->velocity_position(200.0f); // TODO
		id_pick_button->on_click([this, field_type, info, self = id_pick_button]() {
			this->source_data([this, info]() {
				char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
				auto* value = reinterpret_cast<rynx::id*>(data + info.cumulative_offset);
				return value;
			});
			this->m_mode = Mode::IdField_Pick;
			this->source_data_cb([field_type, self](void* data) {
				self->text().text(field_type.m_field_name + ": ^g" + rynx::to_string(*reinterpret_cast<int64_t*>(data)));
			});
		});

		char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
		auto* value = reinterpret_cast<rynx::id*>(data + info.cumulative_offset);
		id_pick_button->text().text(field_type.m_field_name + ": ^g" + rynx::to_string(value->value));
		id_pick_button->align().left_inside().top_inside();

		auto select_pointed_entity_button = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.3f, 1.0f, 0.0f));
		select_pointed_entity_button->align().target(id_pick_button.get()).right_outside().top_inside();
		select_pointed_entity_button->text().text("goto");
		select_pointed_entity_button->velocity_position(200.0f);
		select_pointed_entity_button->on_click([this, target_id = *value]() {
			execute([=]() { this->m_editor_state->m_on_entity_selected(target_id); });
		});

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		field_div->addChild(id_pick_button);
		field_div->addChild(select_pointed_entity_button);
		info.component_sheet->addChild(field_div);
		return true;
	}

	if (type.m_type_name == rynx::traits::type_name<rynx::vec3f>())
	{
		create_vec3f_menu(field_type.m_field_name, info, []() { return rynx::id(); });
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
						if (id) {
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
								if (!game_ecs.exists(id)) {
									return;
								}

								auto entity = game_ecs[id];
								auto& entity_pos = entity.get<rynx::components::transform::position>();
								auto* entity_radius = entity.try_get<rynx::components::transform::radius>();
								bounds.emplace_back(entity_pos.value, entity_radius ? entity_radius->r : 0);
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
								auto& entity_pos = entity.get<rynx::components::transform::position>();

								entity_pos.value = rynx::math::rotatedXY((entity_pos.value - m_drag.middle_point), delta_angle) + m_drag.middle_point;
								entity_pos.angle += delta_angle;

								execute([this, &game_ecs, &ctx, id]() {
									for_each_tool([id, &game_ecs, &ctx](rynx::editor::itool* tool) {
										tool->on_entity_component_value_changed(
											&ctx,
											rynx::type_index::id<rynx::components::transform::position>(),
											game_ecs,
											id);
									});
								});
							}
						}
						else if (gameInput.isKeyDown(rynx::key::codes::control_left()) || gameInput.isKeyDown(rynx::key::codes::control_right())) {
							// drag selection scale
							if ((m_drag.prev_point - m_drag.start_point).length() > 0.5f && position_delta.length() > 1e-5f)
							{
								const auto& ids = selected_id_vector();
								float rescale = 1.0f + 0.003f * position_delta.dot((m_drag.start_point - m_drag.middle_point).normal());
								for (auto id : ids) {
									auto entity = game_ecs[id];
									auto& entity_pos = entity.get<rynx::components::transform::position>();

									rynx::vec3f direction = (entity_pos.value - m_drag.middle_point).normal();
									float length = (entity_pos.value - m_drag.middle_point).length();

									rynx::vec3f scaled_pos = direction * length * rescale + m_drag.middle_point;
									entity_pos.value = scaled_pos;

									auto& entity_radius = entity.get<rynx::components::transform::radius>();
									entity_radius.r *= rescale;

									auto* entity_boundary = entity.try_get<rynx::components::phys::boundary>();
									if (entity_boundary) {
										entity_boundary->segments_local.edit().scale(rescale, rescale);
									}

									execute([this, entity_boundary, &game_ecs, &ctx, id]() {
										for_each_tool([id, entity_boundary, &game_ecs, &ctx](rynx::editor::itool* tool) {
											tool->on_entity_component_value_changed(
												&ctx,
												rynx::type_index::id<rynx::components::transform::position>(),
												game_ecs,
												id);
											
											tool->on_entity_component_value_changed(
												&ctx,
												rynx::type_index::id<rynx::components::transform::radius>(),
												game_ecs,
												id);

											if (entity_boundary) {
												tool->on_entity_component_value_changed(
													&ctx,
													rynx::type_index::id<rynx::components::phys::boundary>(),
													game_ecs,
													id);
											}
										});
									});
								}
							}
						}
						else {
							// drag entity position
							const auto& ids = selected_id_vector();
							for (auto id : ids) {
								auto entity = game_ecs[id];
								auto& entity_pos = entity.get<rynx::components::transform::position>();
								entity_pos.value += position_delta;

								execute([this, &game_ecs, &ctx, id]() {
									for_each_tool([id, &game_ecs, &ctx](rynx::editor::itool* tool) {
										tool->on_entity_component_value_changed(
											&ctx,
											rynx::type_index::id<rynx::components::transform::position>(),
											game_ecs,
											id);
									});
								});
							}
						}
					}
					else if (m_mode == Mode::Vec3fDrag) {
						auto* data = reinterpret_cast<rynx::vec3f*>(address_of_operand());
						if (game_ecs.exists(m_mode_local_space_id)) {
							auto entity = game_ecs[m_mode_local_space_id];
							auto& entity_pos = entity.get<rynx::components::transform::position>();
							position_delta = rynx::math::rotatedXY(position_delta, -entity_pos.angle);
						}
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
	game_ecs.query().for_each([&, mouse_world_pos = cursorWorldPos](rynx::ecs::id id, rynx::components::transform::position pos) {
		float sqr_dist = (mouse_world_pos - pos.value).length_squared();
		auto* boundary_ptr = game_ecs[id].try_get<rynx::components::phys::boundary>();
		auto* radius_ptr = game_ecs[id].try_get<rynx::components::transform::radius>();

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
