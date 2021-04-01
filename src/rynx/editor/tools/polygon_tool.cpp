
#include <rynx/editor/tools/polygon_tool.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/tech/collision_detection.hpp>

void rynx::editor::tools::polygon_tool::update(rynx::scheduler::context& ctx) {
	ctx.add_task("polygon tool tick", [this](
		rynx::ecs& game_ecs,
		rynx::collision_detection& detection,
		rynx::mapped_input& gameInput,
		rynx::camera& gameCamera)
		{
			auto id = selected_id();
			if (game_ecs.exists(id)) {
				auto entity = game_ecs[id];
				if (entity.has<rynx::components::boundary>()) {
					auto mouseRay = gameInput.mouseRay(gameCamera);
					auto [mouse_z_plane, hit] = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
					mouse_z_plane.z = 0;

					if (gameInput.isKeyPressed(m_activation_key)) {
						if (hit) {
							if (!vertex_create(game_ecs, mouse_z_plane)) {
								m_selected_vertex = vertex_select(game_ecs, mouse_z_plane);
							}
							drag_operation_start(game_ecs, mouse_z_plane);
						}
					}

					if (gameInput.isKeyPressed(m_secondary_activation_key)) {
						if (hit) {
							int32_t vertex_index = vertex_select(game_ecs, mouse_z_plane);
							if (vertex_index >= 0) {
								auto& boundary = entity.get<rynx::components::boundary>();
								auto pos = entity.get<rynx::components::position>();
								boundary.segments_local.edit().erase(vertex_index);
								boundary.segments_world = boundary.segments_local;
								boundary.update_world_positions(pos.value, pos.angle);
							}
						}
					}

					if (gameInput.isKeyDown(m_activation_key)) {
						drag_operation_update(game_ecs, mouse_z_plane);
					}

					if (gameInput.isKeyReleased(m_activation_key)) {
						drag_operation_end(game_ecs, detection);
					}

					// smooth selected polygon
					if (gameInput.isKeyClicked(m_key_smooth)) {
						auto& boundary = entity.get<rynx::components::boundary>();
						boundary.segments_local.edit().smooth(3);
						boundary.segments_local.recompute_normals();
						boundary.segments_world = boundary.segments_local;

						auto pos = entity.get<rynx::components::position>();
						boundary.update_world_positions(pos.value, pos.angle);

						// TODO: should really also update radius.
					}
				}
			}
		});
}



bool rynx::editor::tools::polygon_tool::vertex_create(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	float best_distance = 1e30f;
	int best_vertex = -1;
	bool should_create = false;

	// find best selection
	auto entity = game_ecs[this->selected_id()];
	auto radius = entity.get<rynx::components::radius>();
	auto pos = entity.get<rynx::components::position>();
	auto& boundary = entity.get<rynx::components::boundary>();

	for (size_t i = 0; i < boundary.segments_world.size(); ++i) {
		// some threshold for vertex picking
		const auto vertex = boundary.segments_world.segment(i);

		if ((vertex.p1 - cursorWorldPos).length_squared() < best_distance) {
			best_distance = (vertex.p1 - cursorWorldPos).length_squared();
			best_vertex = int(i);
			should_create = false;
		}

		float segment_mid_dist = ((vertex.p1 + vertex.p2) * 0.5f - cursorWorldPos).length_squared();
		if (segment_mid_dist < best_distance) {
			best_distance = segment_mid_dist;
			best_vertex = int(i);
			should_create = true;
		}
	}

	if (should_create) {
		boundary.segments_local.edit().insert(best_vertex, cursorWorldPos - pos.value);
		boundary.segments_world = boundary.segments_local;
		boundary.update_world_positions(pos.value, pos.angle);
		m_selected_vertex = best_vertex + 1;
	}

	return should_create;
}

int32_t rynx::editor::tools::polygon_tool::vertex_select(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	float best_distance = 1e30f;
	int best_vertex = -1;

	// find best selection
	auto entity = game_ecs[selected_id()];
	auto radius = entity.get<rynx::components::radius>();
	auto pos = entity.get<rynx::components::position>();
	const auto& boundary = entity.get<rynx::components::boundary>();

	for (size_t i = 0; i < boundary.segments_world.size(); ++i) {
		// some threshold for vertex picking
		const auto vertex = boundary.segments_world.segment(i);
		float limit = (best_vertex == -1) ? 15.0f * 15.0f : best_distance;
		if ((vertex.p1 - cursorWorldPos).length_squared() < limit) {
			best_distance = (vertex.p1 - cursorWorldPos).length_squared();
			best_vertex = int(i);
		}
	}

	return best_vertex;
}

void rynx::editor::tools::polygon_tool::drag_operation_start(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	m_drag_action_mouse_origin = cursorWorldPos;
	if (m_selected_vertex != -1) {
		auto& boundary = game_ecs[selected_id()].get<rynx::components::boundary>();
		m_drag_action_object_origin = boundary.segments_local.vertex_position(m_selected_vertex);
	}
	else {
		m_drag_action_object_origin = game_ecs[selected_id()].get<rynx::components::position>().value;
	}
}

void rynx::editor::tools::polygon_tool::drag_operation_update(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	if (m_drag_action_active || (cursorWorldPos - m_drag_action_mouse_origin).length_squared() > 10.0f * 10.0f) {
		m_drag_action_active = true;
		auto entity = game_ecs[selected_id()];
		auto& entity_pos = entity.get<rynx::components::position>();
		rynx::vec3f position_delta = cursorWorldPos - m_drag_action_mouse_origin;

		position_delta = rynx::math::rotatedXY(position_delta, -entity_pos.angle);

		if (m_selected_vertex != -1) {
			auto& boundary = entity.get<rynx::components::boundary>();
			auto editor = boundary.segments_local.edit();
			editor.vertex(m_selected_vertex).position(m_drag_action_object_origin + position_delta);

			// todo: just update the edited parts
			boundary.update_world_positions(entity_pos.value, entity_pos.angle);
		}
		else {
			entity_pos.value = m_drag_action_object_origin + position_delta;
		}
	}
}

void rynx::editor::tools::polygon_tool::drag_operation_end(rynx::ecs& game_ecs, rynx::collision_detection& detection) {
	if (m_drag_action_active) {
		auto entity = game_ecs[selected_id()];
		auto& entity_pos = entity.get<rynx::components::position>();
		auto& boundary = entity.get<rynx::components::boundary>();

		/*
		// TODO: add optimize bounding sphere as a tool for boundaries?
		{
			auto editor = boundary.segments_local.edit();
			auto bounding_sphere = boundary.segments_local.bounding_sphere();
			editor.translate(-bounding_sphere.first);
			entity_pos.value += bounding_sphere.first;
		}
		*/

		entity.get<rynx::components::radius>().r = boundary.segments_local.radius();
		boundary.update_world_positions(entity_pos.value, entity_pos.angle);
		detection.update_entity_forced(game_ecs, entity.id());
	}

	m_drag_action_active = false;
}