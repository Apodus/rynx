
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/filesystem/filetree/memfile_directory_node.hpp>
#include <rynx/filesystem/filetree/native_directory_node.hpp>
#include <rynx/filesystem/filetree/native_file_node.hpp>

#include <rynx/system/assert.hpp>
#include <functional>

rynx::filesystem::vfs::~vfs() {
}

rynx::filesystem::vfs::vfs() {
	m_filetree_root = std::make_shared<filetree::node>("");
	native_directory("./", "/wd");
}

bool rynx::filesystem::vfs::file_exists(std::string path) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(path));
	while (true) {
		if (tree_node->file_exists(remaining_path))
			return true;

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + remaining_path;
			tree_node = parent;
		}
		else {
			return false;
		}
	}
}

bool rynx::filesystem::vfs::directory_exists(std::string path) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(path));
	while (true) {
		if (tree_node->directory_exists(remaining_path))
			return true;

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + remaining_path;
			tree_node = parent;
		}
		else {
			return false;
		}
	}
}

std::vector<std::string> rynx::filesystem::vfs::enumerate_vfs_content(std::string virtual_path, rynx::filesystem::recursive recurse, filetree::detail::enumerate_flags flags) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));

	std::vector<std::string> total_result;
	std::function<void(std::shared_ptr<filetree::node>, std::string)> dfs = [this, &total_result, recurse, flags, &dfs](std::shared_ptr<filetree::node> node, std::string current_virtual_path)
	{
		if (current_virtual_path.back() != '/')
			current_virtual_path += "/";

		auto local_result = node->enumerate_content("", recurse, flags);
		std::transform(local_result.begin(), local_result.end(), local_result.begin(), [&current_virtual_path](const std::string& p)->std::string { return current_virtual_path + p; });
		total_result.insert(total_result.end(), local_result.begin(), local_result.end());

		for (const auto& child : node->m_children) {
			if (child.second->is_file_mount()) {
				if (flags.files()) {
					total_result.emplace_back(current_virtual_path + child.second->m_name);
				}
			}
			else {
				dfs(child.second, current_virtual_path + child.second->m_name);
			}
		}
	};

	// if we find and exact match in vfs
	if (remaining_path.empty()) {
		if (recurse == rynx::filesystem::recursive::yes) {
			for (auto child : tree_node->m_children) {
				if (child.second->is_file_mount() && flags.files()) {
					total_result.emplace_back(child.second->m_name);
				}
				else {
					dfs(child.second, child.second->m_name);
				}
			}
		}
		else {
			for (const auto& child : tree_node->m_children) {
				if (child.second->is_file_mount() && flags.files()) {
					total_result.emplace_back(child.second->m_name);
				}
			}
		}
	}

	// also backtrack the vfs path and scan each mount on the way to the root for possible matches.
	while (true) {
		if (tree_node->is_file_mount() && flags.files()) {
			total_result.emplace_back(tree_node->m_name);
		}
		else {
			auto node_result = tree_node->enumerate_content(remaining_path, recurse, flags);
			total_result.insert(total_result.end(), node_result.begin(), node_result.end());
		}

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + remaining_path;
			tree_node = parent;
		}
		else {
			return total_result;
		}
	}
}

std::vector<std::string> rynx::filesystem::vfs::enumerate_files(std::string virtual_path, rynx::filesystem::recursive recurse) const {
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_files });
}

std::vector<std::string> rynx::filesystem::vfs::enumerate_directories(std::string virtual_path, rynx::filesystem::recursive recurse) const {
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_directories });
}

std::vector<std::string> rynx::filesystem::vfs::enumerate(std::string virtual_path, rynx::filesystem::recursive recurse) const {
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_files | rynx::filesystem::filetree::detail::enumerate_flags::flag_directories });
}


std::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::vfs::open_write(std::string virtual_path, filesystem::iwrite_file::mode mode) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	while (true) {
		auto pFile = tree_node->open_write(remaining_path, mode);
		if (pFile)
			return pFile;

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + remaining_path;
			tree_node = parent;
		}
		else {
			return {};
		}
	}
}

std::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::vfs::open_read(std::string virtual_path) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	while (true) {
		auto file = tree_node->open_read(remaining_path);
		if (file) {
			return file;
		}

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + "/" + remaining_path;
			tree_node = parent;
		}
		else {
			return {};
		}
	}
}

bool rynx::filesystem::vfs::remove(std::string virtual_path) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	while (true) {

		if (remaining_path.empty() && tree_node->is_file_mount()) {
			auto parent = tree_node->parent().lock();
			bool result = tree_node->remove("");
			parent->m_children.erase(tree_node->m_name);
			if (result) {
				return true;
			}
		}

		if (tree_node->remove(remaining_path)) {
			return true;
		}

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + "/" + remaining_path;
			tree_node = parent;
		}
		else {
			return false;
		}
	}
}

namespace {
	void remove_backslashes_from_path(std::string& path) {
		std::transform(path.begin(), path.end(), path.begin(), [](char in) { if (in == '\\') return '/'; return in; });
	}
}

void rynx::filesystem::vfs::unmount(std::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	auto [mountNode, remaining_path] = vfs_tree_access(std::move(virtual_path));
	auto parent = mountNode->parent().lock();
	parent->m_children.erase(mountNode->name());
	auto new_child = std::make_shared<filetree::node>(mountNode->name());
	new_child->m_parent = parent;
	parent->m_children.emplace(std::make_pair(mountNode->name(), std::move(new_child)));
	parent->m_children[mountNode->name()]->m_children = mountNode->m_children;
	auto newParent = parent->m_children[mountNode->name()];
	std::for_each(mountNode->m_children.begin(), mountNode->m_children.end(), [&newParent](std::pair<const std::string, std::shared_ptr<filetree::node>>& child) { child.second->m_parent = newParent; });
}

namespace {
	void attach_to_filetree(std::shared_ptr<rynx::filesystem::filetree::node> node_replaced, std::shared_ptr<rynx::filesystem::filetree::node> node_replace_with) {
		// first erase the node where we are about to mount.
		auto parent = node_replaced->m_parent.lock();
		parent->m_children.erase(node_replaced->m_name);
		
		node_replace_with->m_parent = parent;
		node_replace_with->m_name = node_replaced->m_name;
		parent->m_children.emplace(node_replaced->m_name, std::move(node_replace_with));

		// copy the children list of the previous mount node to the new one.
		parent->m_children[node_replaced->m_name]->m_children = node_replaced->m_children;
		
		// update any existing child nodes' parent pointers to just mounted node
		auto new_parent = parent->m_children[node_replaced->m_name];
		std::for_each(
			node_replaced->m_children.begin(),
			node_replaced->m_children.end(),
			[&new_parent](std::pair<const std::string, std::shared_ptr<rynx::filesystem::filetree::node>>& child) {
				child.second->m_parent = new_parent;
			}
		);
	}
}

void rynx::filesystem::vfs::memory_file(std::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	std::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	attach_to_filetree(mountNode, std::make_shared<filetree::memoryfile_node>(mountNode->m_name));
}

void rynx::filesystem::vfs::memory_directory(std::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	std::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	attach_to_filetree(mountNode, std::make_shared<filetree::memory_directory_node>(mountNode->m_name));
}

void rynx::filesystem::vfs::native_directory(std::string native_path, std::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	if (native_path.back() != '/')
		native_path += "/";

	rynx_assert(rynx::filesystem::native::directory_exists(native_path), "mounting native directory '%s' - it does not exist.", native_path.c_str());
	std::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	attach_to_filetree(mountNode,std::make_shared<filetree::native_directory_node>(mountNode->m_name, native_path));
}

void rynx::filesystem::vfs::native_file(std::string native_path, std::string virtual_path) {
	remove_backslashes_from_path(native_path);
	if (native_path.back() == '/')
		native_path.pop_back();

	rynx_assert(rynx::filesystem::native::file_exists(native_path), "mounting native file '%s' - it does not exist.", native_path.c_str());
	std::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	attach_to_filetree(mountNode, std::make_shared<filetree::native_file_node>(mountNode->m_name, native_path));
}

namespace {
	// removes the first path token from given path string
	// returns the first path token as a separate string value
	std::string firstPathToken(std::string& path) {
		auto delim_pos = path.find_first_of("/");
		if (delim_pos != std::string::npos) {
			std::string token = path.substr(0, delim_pos);
			path = path.substr(delim_pos + 1);
			return token;
		}

		std::string token = path;
		path.clear();
		return token;
	}
}

std::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::vfs_tree_mount(std::string virtual_path) const {
	std::shared_ptr<rynx::filesystem::filetree::node> currentNode = m_filetree_root;
	std::string currentTokenName;

	while (true) {
		currentTokenName = firstPathToken(virtual_path);
		if (currentTokenName.empty() || currentTokenName == ".") {
			if (virtual_path.empty())
				return currentNode;
			continue;
		}
		if (currentTokenName == "..") { // reference to parent directory
			rynx_assert(!currentNode->m_parent.expired(), "vfs failed: %s", virtual_path.c_str());
			currentNode = currentNode->m_parent.lock();
			continue;
		}

		auto it = currentNode->m_children.find(currentTokenName);
		if (it != currentNode->m_children.end()) {
			currentNode = it->second;
		}
		else {
			auto emptyNode = std::make_shared<rynx::filesystem::filetree::node>(currentTokenName);
			emptyNode->m_parent = currentNode;
			
			currentNode->m_children.insert(std::make_pair(currentTokenName, std::move(emptyNode)));
			currentNode = currentNode->m_children[currentTokenName];
		}

		if (virtual_path.empty())
			break;
	}

	return currentNode;
}

std::pair<std::shared_ptr<rynx::filesystem::filetree::node>, std::string> rynx::filesystem::vfs::vfs_tree_access(std::string virtual_path) const {
	std::shared_ptr<rynx::filesystem::filetree::node> currentNode = m_filetree_root;
	std::string currentTokenName;
	std::string remaining_path;
	while (true) {
		currentTokenName = firstPathToken(virtual_path);
		if (currentTokenName.empty() || currentTokenName == ".") {
			if(virtual_path.empty())
				return { currentNode, remaining_path };
			continue;
		}
		if (currentTokenName == "..") { // reference to parent directory
			rynx_assert(!currentNode->m_parent.expired(), "vfs failed: %s", virtual_path.c_str());
			currentNode = currentNode->m_parent.lock();
			continue;
		}

		auto it = currentNode->m_children.find(currentTokenName);
		if (it != currentNode->m_children.end()) {
			currentNode = it->second;
		}
		else {
			remaining_path = currentTokenName;
			if (!virtual_path.empty())
				remaining_path += "/" + virtual_path;
			break;
		}

		if (virtual_path.empty())
			break;
	}

	return { currentNode, remaining_path };
}