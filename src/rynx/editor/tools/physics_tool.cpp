#include <rynx/editor/tools/physics_tool.hpp>
#include <rynx/tech/components.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/graphics/mesh/polygon_triangulation.hpp>
#include <rynx/system/typeid.hpp>

void moment_of_inertia(rynx::components::phys::body& b, float r, float scale = 1.0f) {
	b.inv_moment_of_inertia = 1.0f / (0.5f * (1.0f / b.inv_mass) * r * r * scale * scale);
}

void moment_of_inertia(rynx::components::phys::body& b, rynx::polygon& p, float scale = 1.0f) {
	rynx_assert(scale > 0.0f, "scale must be positive non-zero value");
	auto ext = p.extents();
	rynx_assert(ext.z.second - ext.z.first == 0, "assuming 2d xy-plane polys");
	float initial_scale = (ext.x.second - ext.x.first) * (ext.y.second - ext.y.first) / 10000.0f;

	auto tris = rynx::polygon_triangulation().make_triangles(p);
	auto centre_of_mass = tris.centre_of_mass();

	std::vector<rynx::vec3f> points_in_p;
	while (true) {
		points_in_p.clear();

		for (float x = ext.x.first; x <= ext.x.second; x += initial_scale) {
			for (float y = ext.y.first; y <= ext.y.second; y += initial_scale) {
				if (tris.point_in_polygon({ x, y, 0 })) {
					points_in_p.emplace_back(rynx::vec3f(x, y, 0));
				}
			}
		}

		float obj_mass = 1.0f / b.inv_mass;
		float cumulative_inertia = 0.0f;

		if (points_in_p.size() > 1000) {
			float point_mass = obj_mass / points_in_p.size();
			for (auto point : points_in_p) {
				float point_radius = (point - centre_of_mass).length();
				cumulative_inertia += point_mass * point_radius * point_radius * scale * scale;
			}

			b.inv_moment_of_inertia = 1.0f / cumulative_inertia;
			return;
		}

		initial_scale *= 0.5f;
	}
}

rynx::editor::tools::physics_tool::physics_tool(rynx::scheduler::context& ctx)
{
	define_action_no_tool_activate(
		rynx::type_index::id<rynx::components::phys::body>(),
		"update inertia",
		[this](rynx::scheduler::context* ctx)
		{
			entity_id_t id = m_editor_state->m_selected_ids.front();
			auto& ecs = ctx->get_resource<rynx::ecs>();
			auto& body = ecs[id].get<rynx::components::phys::body>();
			auto& radius = ecs[id].get<rynx::components::transform::radius>();
			auto* boundary = ecs[id].try_get<rynx::components::phys::boundary>();

			if (boundary) {
				moment_of_inertia(body, boundary->segments_local, 1.0f);
			}
			else {
				moment_of_inertia(body, radius.r, 1.0f);
			}
		});
}

bool rynx::editor::tools::physics_tool::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	rynx::reflection::type type = info.reflections->get(field_type);
	if (info.component_type_id == rynx::type_index::id<rynx::components::phys::body>())
	{
		return false;
	}
	
	return false;
}

void rynx::editor::tools::physics_tool::update(rynx::scheduler::context&) {}

void rynx::editor::tools::physics_tool::on_tool_selected() {

}
