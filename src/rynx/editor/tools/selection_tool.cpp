
#include <rynx/editor/tools/selection_tool.hpp>
#include <rynx/math/geometry/ray.hpp>

void rynx::editor::tools::selection_tool::update(rynx::scheduler::context& ctx) {
	if (m_run_on_main_thread) {
		m_run_on_main_thread();
		m_run_on_main_thread = nullptr;
	}

	ctx.add_task("editor tick", [this](
		rynx::ecs& game_ecs,
		rynx::mapped_input& gameInput,
		rynx::camera& gameCamera)
		{
			if (gameInput.isKeyPressed(m_activation_key) && !gameInput.isKeyConsumed(m_activation_key)) {
				auto mouseRay = gameInput.mouseRay(gameCamera);
				auto [mouse_z_plane, hit] = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
				mouse_z_plane.z = 0;

				if (hit) {
					on_key_press(game_ecs, mouse_z_plane);
				}
			}
		}
	);
}

void rynx::editor::tools::selection_tool::on_key_press(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos) {
	float best_distance = 1e30f;
	rynx::ecs::id best_id;

	// find best selection
	game_ecs.query().for_each([&, mouse_world_pos = cursorWorldPos](rynx::ecs::id id, rynx::components::position pos) {
		float sqr_dist = (mouse_world_pos - pos.value).length_squared();
		auto* ptr = game_ecs[id].try_get<rynx::components::boundary>();

		if (sqr_dist < best_distance) {
			best_distance = sqr_dist;
			best_id = id;
		}

		if (ptr) {
			for (size_t i = 0; i < ptr->segments_world.size(); ++i) {
				// some threshold for vertex picking
				const auto vertex = ptr->segments_world.segment(i);
				auto [dist, shortest_pos] = rynx::math::pointDistanceLineSegmentSquared(vertex.p1, vertex.p2, mouse_world_pos);
				if (dist < best_distance) {
					best_distance = dist;
					best_id = id;
				}
			}
		}
		});

	// unselect previous selection
	/*
	if (game_ecs.exists(selected_id())) {
		auto* color_ptr = game_ecs[selected_id()].try_get<rynx::components::color>();
		if (color_ptr) {
			color_ptr->value = m_selected_entity_original_color;
		}
	}
	*/

	// select new selection
	{
		/*
		auto* color_ptr = game_ecs[best_id].try_get<rynx::components::color>();
		if (color_ptr) {
			m_selected_entity_original_color = color_ptr->value;
			color_ptr->value = rynx::floats4{ 1.0f, 0.0f, 0.0f, 1.0f };
		}
		*/

		if (m_editor_state->m_on_entity_selected) {
			m_run_on_main_thread = [this, selected_entity = best_id]() {
				m_editor_state->m_on_entity_selected(selected_entity);
			};
		}
		std::cerr << "entity selection tool picked: " << best_id.value << std::endl;
	}
}