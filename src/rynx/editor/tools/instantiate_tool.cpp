
#include <rynx/editor/tools/instantiate_tool.hpp>
#include <rynx/menu/FileSelector.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/math/geometry/plane.hpp>

#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/ecs/ecs.hpp>
#include <rynx/ecs/scenes.hpp>

#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/ecs/scene_serialization.hpp>

rynx::editor::tools::instantiation_tool::instantiation_tool(rynx::scheduler::context& ctx) {
	m_frame_tex_id = ctx.get_resource<rynx::graphics::GPUTextures>().findTextureByName("frame");
	m_vfs = ctx.get_resource_ptr<rynx::filesystem::vfs>();
	m_scenes = ctx.get_resource_ptr<rynx::scenes>();
}

void rynx::editor::tools::instantiation_tool::update(rynx::scheduler::context& ctx) {
	auto& input = ctx.get_resource<rynx::mapped_input>();
	auto& scenes = ctx.get_resource<rynx::scenes>();
	if (input.isKeyClicked(input.getMouseKeyPhysical(0))) {
		auto& gameCam = ctx.get_resource<rynx::camera>();
		auto& ecs = ctx.get_resource<rynx::ecs>();
		auto& reflections = ctx.get_resource<rynx::reflection::reflections>();
		auto [point, hit] = input.mouseRay(gameCam).intersect(rynx::plane(0, 0, 1, 0));

		// if mouse clicked the z-plane
		if (hit) {
			point.z = 0; // the floating point calculations are not 100% accurate, so fix the z value.
			auto scene_id = scenes.filepath_to_info(m_selectedScene).id;
			rynx::id new_scene = ecs.create(rynx::components::transform::position{ point, 0 }, rynx::components::scene::link{ scene_id });
			rynx::entity_range_t deserialized_entities = rynx::ecs_detail::scene_serializer(ecs).load_subscenes(reflections, *m_vfs, scenes);

			for (rynx::id child_entity : deserialized_entities) {
				auto* pos = ecs[child_entity].try_get<rynx::components::transform::position>();
				auto* boundary = ecs[child_entity].try_get<rynx::components::phys::boundary>();
				if (pos && boundary) {
					boundary->segments_world = boundary->segments_local;
					boundary->update_world_positions(pos->value, pos->angle);
				}
			}
		}
	}
}

void rynx::editor::tools::instantiation_tool::on_tool_selected() {
	execute([this]() {
		auto fileselector = rynx::make_shared<rynx::menu::FileSelector>(*m_vfs, m_frame_tex_id, rynx::vec3f(0.5f, 0.5f, 0.0f));
		fileselector->file_type(".rynxscene");
		fileselector->configure().m_allowNewFile = false;
		fileselector->configure().m_allowNewDir = false;
		fileselector->filepath_to_display_name([this](rynx::string path) {
			return  this->m_scenes->filepath_to_info(path).name;
		});

		m_editor_state->m_editor->push_popup(fileselector);
		fileselector->display(
			"/scenes/",
			[this](rynx::string filePath) {
				m_selectedScene = filePath;
				execute([this]() { m_editor_state->m_editor->pop_popup(); });
			},
			[](rynx::string /*dirPath*/) {}
		);
	});
}
