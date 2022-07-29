
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/std/memory.hpp>
#include <rynx/std/string.hpp>

namespace rynx {
	class mapped_input;

	namespace filesystem {
		class vfs;
	}

	namespace menu {

		class MenuDLL FileSelector : public Component {
		protected:
			
		public:
			FileSelector(
				rynx::filesystem::vfs& vfs,
				rynx::graphics::texture_id texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>()
			) : Component(scale, position), m_vfs(vfs)
			{
				m_frame_tex_id = texture;
			}

			struct Config {
				bool m_showFiles = true;
				bool m_showDirs = true;
				bool m_allowNewFile = false;
				bool m_allowNewDir = false;
				bool m_addFileExtensionToNewFiles = false;
			};

			Config& configure() { return m_config; }
			void file_type(rynx::string type) { m_acceptedFileEnding = type; }
			void filepath_to_display_name(rynx::function<rynx::string(rynx::string)> nameMapping) {
				m_pathToName = nameMapping;
			}

			void display(
				rynx::string path,
				rynx::function<void(rynx::string)> on_select_file,
				rynx::function<void(rynx::string)> on_select_directory);

		private:
			void execute(rynx::function<void()> op) {
				m_ops.emplace_back(std::move(op));
			}

			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
			
			Config m_config;
			rynx::filesystem::vfs& m_vfs;
			rynx::string m_currentPath;
			rynx::string m_acceptedFileEnding;
			rynx::graphics::texture_id m_frame_tex_id;
			std::vector<rynx::function<void()>> m_ops;
			rynx::function<rynx::string(rynx::string)> m_pathToName;
		};
	}
}
