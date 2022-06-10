
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/vector.hpp>
#include <functional>
#include <string>

namespace rynx {
	class mapped_input;

	namespace filesystem {
		class vfs;
	}

	namespace menu {

		class FileSelector : public Component {
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
			void file_type(std::string type) { m_acceptedFileEnding = type; }
			void filepath_to_display_name(std::function<std::string(std::string)> nameMapping) {
				m_pathToName = nameMapping;
			}

			void display(
				std::string path,
				std::function<void(std::string)> on_select_file,
				std::function<void(std::string)> on_select_directory);

		private:
			void execute(std::function<void()> op) {
				m_ops.emplace_back(std::move(op));
			}

			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
			
			Config m_config;
			rynx::filesystem::vfs& m_vfs;
			std::string m_currentPath;
			std::string m_acceptedFileEnding;
			rynx::graphics::texture_id m_frame_tex_id;
			std::vector<std::function<void()>> m_ops;
			std::function<std::string(std::string)> m_pathToName;
		};
	}
}
