
#include <rynx/editor/tools/polygon_tool.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/tech/collision_detection.hpp>
#include <rynx/application/components.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/graphics/mesh/collection.hpp>
#include <rynx/graphics/mesh/polygon_triangulation.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/system/typeid.hpp>

rynx::editor::tools::polygon_tool::polygon_tool(rynx::scheduler::context& ctx) {
	auto& input = ctx.get_resource<rynx::mapped_input>();
	m_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(0), "polygon tool activate");
	m_secondary_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(1), "polygon tool activate");
	m_key_smooth = input.generateAndBindGameKey(',', "polygon smooth op");

	define_action(rynx::type_index::id<rynx::components::phys::boundary>(), "vertices edit", [this](rynx::scheduler::context*) {
		// no action required, just a shorthand to activate the tool.
	});

	define_action_no_tool_activate(rynx::type_index::id<rynx::components::phys::boundary>(), "vertices smooth", [this](rynx::scheduler::context* ctx) {
		action_smooth(ctx->get_resource<rynx::ecs>());
	});

	define_action_no_tool_activate(rynx::type_index::id<rynx::components::phys::boundary>(), "mesh rebuild", [this](rynx::scheduler::context* ctx) {
		action_rebuild_mesh(ctx->get_resource<rynx::ecs>(), ctx->get_resource<rynx::graphics::mesh_collection>());
	});
	
	define_action_no_tool_activate(rynx::type_index::id<rynx::components::phys::boundary>(), "mesh rebuild boundary", [this](rynx::scheduler::context* ctx) {
		action_rebuild_boundary_mesh(ctx->get_resource<rynx::ecs>(), ctx->get_resource<rynx::graphics::mesh_collection>());
	});

	// Note that this changes the centre of mass for the entity.
	define_action_no_tool_activate(rynx::type_index::id<rynx::components::phys::boundary>(), "opimize bounding sphere", [this](rynx::scheduler::context* ctx) {
		rynx::ecs& game_ecs = ctx->get_resource<rynx::ecs>();
		auto entity = game_ecs[selected_id()];
		auto& entity_pos = entity.get<rynx::components::transform::position>();
		auto& boundary = entity.get<rynx::components::phys::boundary>();
		auto& radius = entity.get<rynx::components::transform::radius>();

		{
			auto editor = boundary.segments_local.edit();
			auto bounding_sphere = boundary.segments_local.bounding_sphere();
			editor.translate(-bounding_sphere.first);
			entity_pos.value += bounding_sphere.first;
			radius.r = bounding_sphere.second;
		}
	});
}

void rynx::editor::tools::polygon_tool::action_smooth(rynx::ecs& ecs) {
	if (ecs.exists(selected_id())) {
		auto entity = ecs[selected_id()];

		if (entity.has<rynx::components::phys::boundary, rynx::components::transform::position>()) {
			auto& boundary = entity.get<rynx::components::phys::boundary>();
			boundary.segments_local.edit().smooth(3);
			boundary.segments_local.recompute_normals();
			boundary.segments_world = boundary.segments_local;

			auto pos = entity.get<rynx::components::transform::position>();
			boundary.update_world_positions(pos.value, pos.angle);
		}
	}
	
	// TODO: should really also update radius.
}

void rynx::editor::tools::polygon_tool::on_entity_component_added(
	rynx::scheduler::context* ctx,
	[[maybe_unused]] rynx::type_index::type_id_t componentType,
	rynx::ecs& ecs,
	rynx::id id)
{
	auto entity = ecs[id];
	if (!entity.has<rynx::components::phys::boundary, rynx::components::transform::position, rynx::components::transform::radius>())
		return;

	auto& boundary = entity.get<rynx::components::phys::boundary>();
	auto& pos = entity.get<rynx::components::transform::position>();
	auto& radius = entity.get<rynx::components::transform::radius>();

	if (boundary.segments_local.size() == 0) {
		boundary.segments_local = Shape::makeCircle(5, 3);
		boundary.segments_world = boundary.segments_local;
		boundary.update_world_positions(pos.value, pos.angle);
		radius.r = boundary.segments_local.radius();
	}

	auto& detection = ctx->get_resource<rynx::collision_detection>();
	detection.editor_api().update_collider_kind_for_entity(ecs, id);
}

void rynx::editor::tools::polygon_tool::on_entity_component_value_changed(
	rynx::scheduler::context* /* ctx */,
	rynx::type_index::type_id_t type_id,
	rynx::ecs& ecs,
	rynx::id id)
{
	if (type_id == rynx::type_index::id<rynx::components::transform::position>()) {
		auto* position_ptr = ecs[id].try_get<rynx::components::transform::position>();
		auto* boundary_ptr = ecs[id].try_get<rynx::components::phys::boundary>();
		if (position_ptr && boundary_ptr) {
			boundary_ptr->update_world_positions(position_ptr->value, position_ptr->angle);
		}
	}
}

void rynx::editor::tools::polygon_tool::on_entity_component_removed(
	rynx::scheduler::context* /* ctx */,
	rynx::type_index::type_id_t /* componentType */,
	rynx::ecs& /* ecs */,
	rynx::id /* id */)
{
}

void rynx::editor::tools::polygon_tool::action_rebuild_mesh(rynx::ecs& ecs, rynx::graphics::mesh_collection& meshes) {
	if (ecs.exists(selected_id())) {
		auto entity = ecs[selected_id()];
		
		if (entity.has<rynx::components::phys::boundary, rynx::components::transform::position>()) {
			auto& boundary = entity.get<rynx::components::phys::boundary>();
			
			// TODO: Mesh name should be selected much better
			auto id = meshes.create("user_polymesh", rynx::polygon_triangulation().make_mesh(boundary.segments_local));
			if (entity.has<rynx::components::graphics::mesh>()) {
				entity.remove<rynx::components::graphics::mesh>();
			}
			entity.add(rynx::components::graphics::mesh(id));
			
			if (!entity.has<rynx::components::graphics::texture>()) {
				entity.add(rynx::components::graphics::texture());
			}

			// refresh the components list
			execute([this]() {m_editor_state->m_on_entity_selected(selected_id()); });
		}
	}
}

void rynx::editor::tools::polygon_tool::action_rebuild_boundary_mesh(rynx::ecs& ecs, rynx::graphics::mesh_collection& meshes) {
	if (ecs.exists(selected_id())) {
		auto entity = ecs[selected_id()];

		if (entity.has<rynx::components::phys::boundary, rynx::components::transform::position>()) {
			auto& boundary = entity.get<rynx::components::phys::boundary>();

			// TODO: Mesh name should be selected much better
			auto id = meshes.create("user_polymesh", rynx::polygon_triangulation().make_boundary_mesh(boundary.segments_local));
			if (entity.has<rynx::components::graphics::mesh>()) {
				entity.remove<rynx::components::graphics::mesh>();
			}
			entity.add(rynx::components::graphics::mesh(id));
		}

		if (!entity.has<rynx::components::graphics::texture>()) {
			entity.add(rynx::components::graphics::texture());
		}

		// refresh the components list
		execute([this]() {m_editor_state->m_on_entity_selected(selected_id()); });
	}
}

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
				if (!entity.has<rynx::components::phys::boundary>()) {
					return;
				}

				if (entity.has<rynx::components::phys::boundary>()) {
					auto mouseRay = gameInput.mouseRay(gameCamera);
					auto [mouse_z_plane, hit] = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
					mouse_z_plane.z = 0;

					float cameraHeight = gameCamera.position().z;
					float pickingDistanceThreshold = cameraHeight * 0.02f;

					{
						auto& boundary = entity.get<rynx::components::phys::boundary>();
						auto& pos = entity.get<rynx::components::transform::position>();
						auto& radius = entity.get<rynx::components::transform::radius>();

						if (boundary.segments_local.size() == 0) {
							boundary.segments_local = Shape::makeCircle(cameraHeight * 0.1f, 3);
							boundary.segments_world = boundary.segments_local;
							boundary.update_world_positions(pos.value, pos.angle);
							radius.r = boundary.segments_local.radius();
						}

						detection.editor_api().update_collider_kind_for_entity(game_ecs, id);
					}

					if (gameInput.isKeyPressed(m_activation_key)) {
						if (hit) {
							m_selected_vertex = vertex_select(game_ecs, mouse_z_plane, pickingDistanceThreshold);
							if (m_selected_vertex == -1) {
								if (!vertex_create(game_ecs, mouse_z_plane, pickingDistanceThreshold)) {
									// maybe drag the entire polygon then.
								}
							}
							drag_operation_start(game_ecs, mouse_z_plane);
						}
					}

					if (gameInput.isKeyPressed(m_secondary_activation_key)) {
						if (hit) {
							int32_t vertex_index = vertex_select(game_ecs, mouse_z_plane, pickingDistanceThreshold);
							if (vertex_index >= 0) {
								auto& boundary = entity.get<rynx::components::phys::boundary>();
								
								// deleting vertices to less than three not allowed.
								if (boundary.segments_local.size() > 3) {
									auto pos = entity.get<rynx::components::transform::position>();
									boundary.segments_local.edit().erase(vertex_index);
									boundary.segments_world = boundary.segments_local;
									boundary.update_world_positions(pos.value, pos.angle);
								}
							}
						}
					}

					if (gameInput.isKeyDown(m_activation_key)) {
						drag_operation_update(game_ecs, mouse_z_plane, pickingDistanceThreshold * 1.5f);
					}

					if (gameInput.isKeyReleased(m_activation_key)) {
						drag_operation_end(game_ecs, detection);
					}

					// smooth selected polygon
					if (gameInput.isKeyClicked(m_key_smooth)) {
						action_smooth(game_ecs);
					}
				}
			}
		});
}



bool rynx::editor::tools::polygon_tool::vertex_create(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float allowedDistance) {
	int best_vertex = -1;
	bool should_create = false;

	// find best selection
	auto entity = game_ecs[this->selected_id()];
	auto radius = entity.get<rynx::components::transform::radius>();
	auto pos = entity.get<rynx::components::transform::position>();
	auto& boundary = entity.get<rynx::components::phys::boundary>();

	float best_distance = allowedDistance;
	for (size_t i = 0; i < boundary.segments_world.size(); ++i) {
		const auto vertex = boundary.segments_world.segment(i);
		auto [distSqr, closestPoint] = rynx::math::pointDistanceLineSegmentSquared(vertex.p1, vertex.p2, cursorWorldPos);
		if (distSqr < best_distance) {
			best_distance = distSqr;
			best_vertex = int(i);
			should_create = true;
		}
	}

	if (should_create) {
		rynx::vec3f local_position = cursorWorldPos - pos.value;
		local_position = rynx::math::rotatedXY(local_position, -pos.angle);

		boundary.segments_local.edit().insert(best_vertex, local_position);
		boundary.segments_world = boundary.segments_local;
		boundary.update_world_positions(pos.value, pos.angle);
		m_selected_vertex = best_vertex + 1;
	}

	return should_create;
}

int32_t rynx::editor::tools::polygon_tool::vertex_select(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float allowedDistance) {
	float best_distance = 1e30f;
	int best_vertex = -1;

	// find best selection
	auto entity = game_ecs[selected_id()];
	auto radius = entity.get<rynx::components::transform::radius>();
	auto pos = entity.get<rynx::components::transform::position>();
	const auto& boundary = entity.get<rynx::components::phys::boundary>();

	for (size_t i = 0; i < boundary.segments_world.size(); ++i) {
		// some threshold for vertex picking
		const auto vertex = boundary.segments_world.segment(i);
		float limit = (best_vertex == -1) ? allowedDistance : best_distance;
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
		auto& boundary = game_ecs[selected_id()].get<rynx::components::phys::boundary>();
		m_drag_action_object_origin = boundary.segments_local.vertex_position(m_selected_vertex);
	}
	else {
		m_drag_action_object_origin = game_ecs[selected_id()].get<rynx::components::transform::position>().value;
	}
}

void rynx::editor::tools::polygon_tool::drag_operation_update(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float activationDistance) {
	if (m_drag_action_active || (cursorWorldPos - m_drag_action_mouse_origin).length_squared() > activationDistance) {
		m_drag_action_active = true;
		auto entity = game_ecs[selected_id()];
		auto& entity_pos = entity.get<rynx::components::transform::position>();
		rynx::vec3f position_delta = cursorWorldPos - m_drag_action_mouse_origin;

		position_delta = rynx::math::rotatedXY(position_delta, -entity_pos.angle);

		if (m_selected_vertex != -1) {
			auto& boundary = entity.get<rynx::components::phys::boundary>();
			auto editor = boundary.segments_local.edit();
			editor.vertex(m_selected_vertex).position(m_drag_action_object_origin + position_delta);

			// todo: just update the edited parts
			boundary.update_world_positions(entity_pos.value, entity_pos.angle);
		}
		else {
			entity_pos.value = m_drag_action_object_origin + position_delta;
			
			auto& boundary = entity.get<rynx::components::phys::boundary>();
			boundary.update_world_positions(entity_pos.value, entity_pos.angle);
		}
	}
}

void rynx::editor::tools::polygon_tool::drag_operation_end(rynx::ecs& game_ecs, rynx::collision_detection& detection) {
	if (m_drag_action_active) {
		auto entity = game_ecs[selected_id()];
		auto& entity_pos = entity.get<rynx::components::transform::position>();
		auto& boundary = entity.get<rynx::components::phys::boundary>();

		entity.get<rynx::components::transform::radius>().r = boundary.segments_local.radius();
		boundary.update_world_positions(entity_pos.value, entity_pos.angle);
		detection.editor_api().update_entity_forced(game_ecs, entity.id());
	}

	m_drag_action_active = false;
}
