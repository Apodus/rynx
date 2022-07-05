
#include <rynx/editor/tools/instantiate_tool.hpp>
#include <rynx/menu/FileSelector.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/math/geometry/plane.hpp>

#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/tech/ecs.hpp>
#include <rynx/tech/ecs/scenes.hpp>

#include <rynx/graphics/texture/texturehandler.hpp>

rynx::editor::tools::instantiation_tool::instantiation_tool(rynx::scheduler::context& ctx) {
	m_frame_tex_id = ctx.get_resource<rynx::graphics::GPUTextures>().findTextureByName("frame");
	m_vfs = ctx.get_resource_ptr<rynx::filesystem::vfs>();
}

void rynx::editor::tools::instantiation_tool::update(rynx::scheduler::context& ctx) {
	auto& input = ctx.get_resource<rynx::mapped_input>();

	if (input.isKeyClicked(input.getMouseKeyPhysical(0))) {
		auto& gameCam = ctx.get_resource<rynx::camera>();
		auto& ecs = ctx.get_resource<rynx::ecs>();
		auto& reflections = ctx.get_resource<rynx::reflection::reflections>();
		auto [point, hit] = input.mouseRay(gameCam).intersect(rynx::plane(0, 0, 1, 0));

		// if mouse clicked the z-plane
		if (hit) {
			point.z = 0; // the floating point calculations are not 100% accurate, so fix the z value.

			auto fileIn = m_vfs->open_read(m_selectedScene);
			rynx::serialization::vector_reader reader(
				ctx.get_resource<rynx::scenes>().read_payload(*fileIn).second
			);
			auto ids = ecs.deserialize(reflections, reader);

			// translate to cursor
			for (auto id : ids) {
				auto* pos = ecs[id].try_get<rynx::components::position>();
				if (pos) {
					pos->value += point;

					auto* boundary = ecs[id].try_get<rynx::components::boundary>();
					if (boundary) {
						boundary->update_world_positions(pos->value, pos->angle);
					}
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
	
		m_editor_state->m_editor->push_popup(fileselector);
		fileselector->display(
			"/scenes/",
			[this](rynx::string filePath) {
				m_selectedScene = filePath;
				execute([this]() { m_editor_state->m_editor->pop_popup(); });
			},
			[](rynx::string dirPath) {}
		);
	});
}
