
#include <rynx/menu/FileSelector.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <algorithm>

void rynx::menu::FileSelector::display(
	rynx::string path,
	rynx::function<void(rynx::string)> on_select_file,
	rynx::function<void(rynx::string)> on_select_directory)
{
	clear_children();

	m_currentPath = path;
	if (m_currentPath.ends_with("/")) {
		m_currentPath.pop_back();
	}

	auto header = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 1.0f, 0.05f, 0.0f });
	header->text("Current path: " + m_currentPath + "/");
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

	if (m_config.m_allowNewFile) {
		createSelection("", "New file", {1.0f, 1.0f, 0.0f, 1.0f}, [this, on_select_file](rynx::string) {
			clear_children();
			
			auto popup_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 1.0f, 0.2f, 0.0f });
			addChild(popup_div);

			auto popup_topic = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 0.5f, 0.35f, 0.0f });
			popup_topic->align().top_inside();
			popup_topic->text("Enter file name:");
			popup_topic->color({ 1.0f, 0.0f, 1.0f, 1.0f });

			auto fileNameButton = rynx::make_shared<rynx::menu::Button>(m_frame_tex_id, rynx::vec3f{ 0.8f, 0.4f, 0.0f }, rynx::vec3f(0, 0, 0));
			fileNameButton->no_focus_alpha(1.0f);
			fileNameButton->align().bottom_inside();
			fileNameButton->text().text_input_enable();
			fileNameButton->text().on_value_changed([this, on_select_file](rynx::string fileName) {
				rynx::string new_file_path = m_currentPath + "/" + fileName;
				if (m_config.m_addFileExtensionToNewFiles) {
					new_file_path += m_acceptedFileEnding;
				}
				execute([new_file_path, on_select_file]() {
					on_select_file(new_file_path);
				});
			});

			popup_div->addChild(fileNameButton);
			popup_div->addChild(popup_topic);
			execute([fileNameButton]() {
				fileNameButton->text().capture_dedicated_keyboard_input();
			});
		});
	}

	// TODO allow creating directories through the vfs
	/*
	if (m_config.m_allowNewDir) {
		createSelection(m_currentPath, "New directory", { 1.0f, 1.0f, 0.0f, 1.0f }, [this, on_select_file, on_select_directory](rynx::string) {
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
			fileNameButton->text().on_value_changed([this, on_select_file, on_select_directory](rynx::string dirName) {
				execute([this, dirName, on_select_file, on_select_directory]() {
					rynx::filesystem::create_directory(m_currentPath + dirName);
					display(m_currentPath + dirName, on_select_file, on_select_directory);
					on_select_directory(m_currentPath + dirName);
				});
			});

			popup_div->addChild(fileNameButton);
			popup_div->addChild(popup_topic);
			execute([fileNameButton]() {
				fileNameButton->text().capture_dedicated_keyboard_input();
			});
		});
	}
	*/

	if (m_config.m_showDirs) {
		auto directory_selected_wrapper = [this, on_select_file, on_select_directory](rynx::string path) {
			execute([this, on_select_file, on_select_directory, path]() {
				display(path, on_select_file, on_select_directory);
				on_select_directory(path);
			});
		};

		{
			rynx::string parentDir;
			auto pos = m_currentPath.find_last_of('/');
			if (pos == rynx::string::npos || (pos == 0)) {
				parentDir = "/";
			}
			else {
				parentDir = m_currentPath.substr(0, pos);
			}

			createSelection(parentDir, "..", { 0.2f, 0.6f, 1.0f, 1.0f }, directory_selected_wrapper);
		}

		std::vector<rynx::string> directorypaths = m_vfs.enumerate_directories(m_currentPath);
		std::sort(directorypaths.begin(), directorypaths.end());
		for (auto&& dirpath : directorypaths) {
			// is current directory. dirpath will be in form path/to/  while m_currentPath is missing the trailing slash.
			if (m_currentPath.size() == dirpath.size() - 1) {
				createSelection(dirpath, ".", { 0.2f, 0.6f, 1.0f, 1.0f }, directory_selected_wrapper);
			}
			else {
				dirpath.pop_back();
				auto pos = dirpath.find_last_of('/');
				dirpath.push_back('/');
				rynx::string displayName = dirpath;
				if (pos != rynx::string::npos) {
					displayName = dirpath.substr(pos + 1);
				}
				rynx::filesystem::resolve_path(dirpath);
				createSelection(dirpath, displayName, { 0.2f, 0.6f, 1.0f, 1.0f }, directory_selected_wrapper);
			}
		}
	}

	if (m_config.m_showFiles) {
		std::vector<rynx::string> filepaths = m_vfs.enumerate_files(m_currentPath);
		std::sort(filepaths.begin(), filepaths.end());
		for (auto&& filepath : filepaths) {
			if (!filepath.ends_with(m_acceptedFileEnding))
				continue;
			
			rynx::string displayName;
			if (m_pathToName) {
				displayName = m_pathToName(filepath);
			}
			else {
				auto pos = filepath.find_last_of('/');
				displayName = filepath;
				if (pos != rynx::string::npos) {
					displayName = filepath.substr(pos + 1);
				}
			}
			createSelection(filepath, displayName, { 0.5f, 1.0f, 0.2f, 1.0f }, on_select_file);
		}
	}
}

void rynx::menu::FileSelector::onInput(rynx::mapped_input&) {}
void rynx::menu::FileSelector::draw(rynx::graphics::renderer&) const {}

void rynx::menu::FileSelector::update(float /* dt */) {
	size_t i = 0;
	while(i < m_ops.size())
		m_ops[i++]();
	m_ops.clear();
}
