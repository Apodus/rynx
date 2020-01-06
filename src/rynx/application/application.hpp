#pragma once

#include <memory>
#include <string>

class GPUTextures;
class Window;
class UserIO;

namespace rynx {
	class MeshRenderer;
	class TextRenderer;

	namespace graphics {
		class shaders;
	}

	namespace application {

		class Application {
		public:
			Application() {}

			void openWindow(int width = 1000, int height = 1000, std::string name = "Rynx Application");
			void loadTextures(std::string path);
			void startFrame();
			void swapBuffers();

			float aspectRatio() const;
			bool isExitRequested() const;
			void set_gl_viewport_to_window_dimensions() const;

			std::shared_ptr<Window> window() { return m_window; }
			std::shared_ptr<UserIO> input() { return m_input; }
			std::shared_ptr<rynx::graphics::shaders> shaders() { return m_shaders; }

			MeshRenderer& meshRenderer() { return *m_meshRenderer; }
			TextRenderer& textRenderer() { return *m_textRenderer; }

			std::shared_ptr<GPUTextures> textures() { return m_textures; }
			
		private:
			std::shared_ptr<Window> m_window;
			std::shared_ptr<UserIO> m_input;
			std::shared_ptr<GPUTextures> m_textures;
			std::shared_ptr<rynx::graphics::shaders> m_shaders;

			std::shared_ptr<MeshRenderer> m_meshRenderer;
			std::shared_ptr<TextRenderer> m_textRenderer;
		};
	}
}
