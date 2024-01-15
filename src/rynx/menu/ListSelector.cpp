
#include <rynx/menu/ListSelector.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/std/string.hpp>
#include <algorithm>

void rynx::menu::ListSelector::display(rynx::function<void(rynx::string)> on_select_item)
{
	if (!m_current_category)
		m_current_category = &m_options_tree;

	clear_children();

	auto header = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 1.0f, 0.05f, 0.0f });
	header->text("List select");
	header->align().top_outside().left_inside();
	header->text_align_left();
	header->velocity_position(100.0f);
	addChild(header);

	auto selection_list = rynx::make_shared<rynx::menu::List>(m_frame_tex_id, rynx::vec3f(1.0f, 1.0f, 0.0f));
	selection_list->velocity_position(100.0f);
	selection_list->list_element_velocity(150.0f);
	selection_list->align().target(header.get()).bottom_outside().left_inside();
	addChild(selection_list);
	
	auto createSelection = [this, selection_list](rynx::string fullPath, rynx::string displayName, rynx::floats4 color, rynx::function<void(rynx::string)> on_select) {
		auto button = rynx::make_shared<rynx::menu::Button>(m_frame_tex_id, rynx::vec3f(1.f, 0.05f, 0.0f));
		button->no_focus_alpha(0.76f);
		button->text().text(displayName);
		button->text().text_align_left();
		button->text().color() = color;
		button->on_click([this, fullPath, on_select]() {
			execute([this, fullPath, on_select]() {
				on_select(fullPath);
			});
		});

		selection_list->addChild(button);
	};

	if (m_config.m_allowNewOption) {
		createSelection("new", "Create new", { 1.0f, 1.0f, 0.0f, 1.0f }, [this, on_select_item](rynx::string) {
			clear_children();
			
			auto popup_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 1.0f, 0.2f, 0.0f });
			addChild(popup_div);

			auto popup_topic = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 0.5f, 0.35f, 0.0f });
			popup_topic->align().top_inside();
			popup_topic->text("Enter name:");
			popup_topic->color({ 1.0f, 0.0f, 1.0f, 1.0f });

			auto fileNameButton = rynx::make_shared<rynx::menu::Button>(m_frame_tex_id, rynx::vec3f{ 0.8f, 0.4f, 0.0f }, rynx::vec3f(0, 0, 0));
			fileNameButton->no_focus_alpha(1.0f);
			fileNameButton->align().bottom_inside();
			fileNameButton->text().text_input_enable();
			fileNameButton->text().on_value_changed([this, on_select_item](rynx::string optionName) {
				execute([this, optionName, on_select_item]() {
					on_select_item(optionName);
				});
			});

			popup_div->addChild(fileNameButton);
			popup_div->addChild(popup_topic);
			execute([fileNameButton]() {
				fileNameButton->text().capture_dedicated_keyboard_input();
			});
		});
	}

	if (m_config.m_allowNewCategory) {
		createSelection("new_cat", "New directory", {1.0f, 1.0f, 0.0f, 1.0f}, [this, on_select_item](rynx::string) {
			clear_children();

			auto popup_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 1.0f, 0.2f, 0.0f });
			addChild(popup_div);

			auto popup_topic = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 0.5f, 0.35f, 0.0f });
			popup_topic->align().top_inside();
			popup_topic->text("Enter directory name:");
			popup_topic->color({ 1.0f, 0.0f, 1.0f, 1.0f });

			auto fileNameButton = rynx::make_shared<rynx::menu::Button>(m_frame_tex_id, rynx::vec3f{ 0.8f, 0.4f, 0.0f }, rynx::vec3f(0, 0, 0));
			fileNameButton->no_focus_alpha(1.0f);
			fileNameButton->align().bottom_inside();
			fileNameButton->text().text_input_enable();
			fileNameButton->text().on_value_changed([this, on_select_item](rynx::string dirName) {
				execute([this, dirName, on_select_item]() {
					// todo: ..
					this->m_current_category = &m_current_category->find(dirName);
					display(on_select_item);
				});
			});

			popup_div->addChild(fileNameButton);
			popup_div->addChild(popup_topic);
			execute([fileNameButton]() {
				fileNameButton->text().capture_dedicated_keyboard_input();
			});
		});
	}

	if (true) {
		auto directory_selected_wrapper = [this, on_select_item](rynx::string path) {
			execute([this, on_select_item, path]() {
				display(on_select_item);
			});
		};

		{
			createSelection("..", "../", { 0.2f, 0.6f, 1.0f, 1.0f }, [this, on_select_item](rynx::string a) {
				if (m_current_category->m_parent) {
					m_current_category = m_current_category->m_parent;
				}
				display(on_select_item);
			});
		}

		std::vector<rynx::string> directorypaths;
		for (auto& cat : m_current_category->m_categories)
			directorypaths.emplace_back(cat->m_name);

		std::sort(directorypaths.begin(), directorypaths.end());
		for (auto&& dirpath : directorypaths) {
			createSelection(dirpath, dirpath, { 0.2f, 0.6f, 1.0f, 1.0f }, directory_selected_wrapper);
		}
	}

	if (true) {
		std::vector<rynx::string> filepaths = m_current_category->m_options;
		std::sort(filepaths.begin(), filepaths.end());
		for (auto&& filepath : filepaths) {
			createSelection(filepath, filepath, { 0.5f, 1.0f, 0.2f, 1.0f }, on_select_item);
		}
	}
}

void rynx::menu::ListSelector::onInput(rynx::mapped_input&) {}
void rynx::menu::ListSelector::draw(rynx::graphics::renderer&) const {}

void rynx::menu::ListSelector::update(float /* dt */) {
	size_t i = 0;
	while(i < m_ops.size())
		m_ops[i++]();
	m_ops.clear();
}
