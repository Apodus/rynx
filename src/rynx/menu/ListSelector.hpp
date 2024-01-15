
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/std/string.hpp>
#include <vector>

namespace rynx {
	class mapped_input;

	namespace menu {

		class MenuDLL ListSelector : public Component {
		protected:
			
		public:
			struct Category {
				Category& add_category(rynx::string name) {
					m_categories.emplace_back(rynx::make_unique<Category>());
					m_categories.back()->m_parent = this;
					return *m_categories.back();
				}

				Category& add_option(rynx::string option) {
					m_options.emplace_back(std::move(option));
					return *this;
				}

				Category& find(rynx::string str) {
					for (auto& cat : m_categories)
						if (cat->m_name == str)
							return *cat;
					return *this;
				}

				rynx::string m_name;
				Category* m_parent = nullptr;
				std::vector<rynx::string> m_options;
				std::vector<rynx::unique_ptr<Category>> m_categories;
			};

			struct Config {
				bool m_allowNewOption = false;
				bool m_allowNewCategory = false;
				bool m_allowCancel = false;
			};

			ListSelector(
				rynx::graphics::texture_id texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>()
			) : Component(scale, position)
			{
				m_frame_tex_id = texture;
			}



			void display(rynx::function<void(rynx::string)> on_select_option);

			Category& category_setup() {
				return m_options_tree;
			}

			Config m_config;

		private:
			void execute(rynx::function<void()> op) {
				m_ops.emplace_back(std::move(op));
			}

			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
			
			Category m_options_tree;
			Category* m_current_category = nullptr;

			rynx::graphics::texture_id m_frame_tex_id;
			std::vector<rynx::function<void()>> m_ops;
		};
	}
}
