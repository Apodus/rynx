
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/vector.hpp>
#include <functional>
#include <string>

namespace rynx {
	class mapped_input;

	namespace menu {

		class FileSelector : public Component {
		protected:
			
		public:
			FileSelector(
				rynx::graphics::texture_id texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>(),
				float frame_edge_percentage = 0.2f
			) : Component(scale, position)
			{
				m_frame_tex_id = texture;
			}

			struct Config {
				bool m_showFiles = true;
				bool m_showDirs = true;
				bool m_allowNewFile = false;
				bool m_allowNewDir = false;
			};

			Config& configure() { return m_config; }
			void file_type(std::string type) { m_acceptedFileEnding = type; }

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
			std::string m_currentPath;
			std::string m_acceptedFileEnding;
			rynx::graphics::texture_id m_frame_tex_id;
			std::vector<std::function<void()>> m_ops;
		};
	}
}
