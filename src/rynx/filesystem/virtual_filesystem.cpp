
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/filesystem/filetree/memfile_directory_node.hpp>
#include <rynx/filesystem/filetree/native_directory_node.hpp>
#include <rynx/filesystem/filetree/native_file_node.hpp>
#include <rynx/filesystem/filetree/memory_file_node.hpp>
#include <rynx/filesystem/filekinds/compressedfile.hpp>

#include <rynx/system/assert.hpp>
#include <rynx/std/memory.hpp> // todo - should include rynx::function

rynx::filesystem::vfs::~vfs() {
}

rynx::filesystem::vfs::vfs() {
	m_filetree_root = rynx::make_shared<filetree::node>("");
	native_directory("./", "/wd");
}

bool rynx::filesystem::vfs::file_exists(rynx::string path) const {
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

bool rynx::filesystem::vfs::directory_exists(rynx::string path) const {
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

std::vector<rynx::string> rynx::filesystem::vfs::enumerate_vfs_content(
	const rynx::string virtual_path,
	rynx::filesystem::recursive recurse,
	filetree::detail::enumerate_flags flags) const {
	auto [tree_node, remaining_path] = vfs_tree_access(virtual_path);

	std::vector<rynx::string> total_result;
	rynx::function<void(rynx::shared_ptr<filetree::node>, rynx::string)> dfs = [this, &total_result, recurse, flags, &dfs](rynx::shared_ptr<filetree::node> node, rynx::string current_virtual_path)
	{
		if (current_virtual_path.back() != '/')
			current_virtual_path += "/";

		auto local_result = node->enumerate_content("", recurse, flags);
		std::transform(local_result.begin(), local_result.end(), local_result.begin(), [&current_virtual_path](const rynx::string& p)->rynx::string { return current_virtual_path + p; });
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
				if (child.second->is_file_mount()) {
					if (flags.files()) {
						total_result.emplace_back(child.second->m_name);
					}
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
				else if(flags.directories()) {
					total_result.emplace_back(child.second->m_name + "/");
				}
			}
		}
	}

	// also backtrack the vfs path and scan each mount on the way to the root for possible matches.
	while (true) {
		auto node_result = tree_node->enumerate_content(remaining_path, recurse, flags);
		total_result.insert(total_result.end(), node_result.begin(), node_result.end());

		auto parent = tree_node->parent().lock();
		if (parent) {
			remaining_path = tree_node->name() + remaining_path;
			tree_node = parent;
		}
		else {
			// complete the result paths to be absolute paths in terms of vfs.
			std::transform(
				total_result.begin(),
				total_result.end(),
				total_result.begin(),
				[&virtual_path](const rynx::string& s) {
					if (virtual_path.ends_with("/") && s == "/")
						return virtual_path;
					return virtual_path + s;
				});
			return total_result;
		}
	}
}

std::vector<rynx::string> rynx::filesystem::vfs::enumerate_files(rynx::string virtual_path, rynx::filesystem::recursive recurse) const {
	if (virtual_path.empty() || virtual_path.back() != '/')
		virtual_path.push_back('/');
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_files });
}

std::vector<rynx::string> rynx::filesystem::vfs::enumerate_directories(rynx::string virtual_path, rynx::filesystem::recursive recurse) const {
	if (virtual_path.empty() || virtual_path.back() != '/')
		virtual_path.push_back('/');
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_directories });
}

std::vector<rynx::string> rynx::filesystem::vfs::enumerate(rynx::string virtual_path, rynx::filesystem::recursive recurse) const {
	if (virtual_path.empty() || virtual_path.back() != '/')
		virtual_path.push_back('/');
	return enumerate_vfs_content(std::move(virtual_path), recurse, { rynx::filesystem::filetree::detail::enumerate_flags::flag_files | rynx::filesystem::filetree::detail::enumerate_flags::flag_directories });
}


rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::vfs::open_write(rynx::string virtual_path, filesystem::iwrite_file::mode mode) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	while (true) {
		auto pFile = tree_node->open_write_with_settings(remaining_path, mode);
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

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::vfs::open_read(rynx::string virtual_path) const {
	auto [tree_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	while (true) {
		auto file = tree_node->open_read_with_settings(remaining_path);
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

bool rynx::filesystem::vfs::remove(rynx::string virtual_path) const {
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
	void remove_backslashes_from_path(rynx::string& path) {
		std::transform(path.begin(), path.end(), path.begin(), [](char in) { if (in == '\\') return '/'; return in; });
	}
}

void rynx::filesystem::vfs::unmount(rynx::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	auto [mount_node, remaining_path] = vfs_tree_access(std::move(virtual_path));
	auto parent = mount_node->parent().lock();
	parent->m_children.erase(mount_node->name());
	auto new_child = rynx::make_shared<filetree::node>(mount_node->name());
	new_child->m_parent = parent;
	parent->m_children.emplace(std::make_pair(mount_node->name(), std::move(new_child)));
	parent->m_children[mount_node->name()]->m_children = mount_node->m_children;
	auto new_parent = parent->m_children[mount_node->name()];
	for (auto&& mount_node_child : mount_node->m_children)
		mount_node_child.second->m_parent = new_parent;
}

namespace {
	auto attach_to_filetree(rynx::shared_ptr<rynx::filesystem::filetree::node> node_replaced, rynx::shared_ptr<rynx::filesystem::filetree::node> node_replace_with) {
		// first erase the node where we are about to mount.
		auto parent = node_replaced->m_parent.lock();
		if (!parent) {
			return rynx::shared_ptr<rynx::filesystem::filetree::node>();
		}
		
		parent->m_children.erase(node_replaced->m_name);
		
		node_replace_with->m_parent = parent;
		node_replace_with->m_name = node_replaced->m_name;
		parent->m_children.emplace(node_replaced->m_name, std::move(node_replace_with));

		// copy the children list of the previous mount node to the new one.
		parent->m_children[node_replaced->m_name]->m_children = node_replaced->m_children;
		
		// update any existing child nodes' parent pointers to just mounted node
		auto new_parent = parent->m_children[node_replaced->m_name];
		for (auto&& replaced_node_child : node_replaced->m_children)
			replaced_node_child.second->m_parent = new_parent;
		return parent->m_children[node_replaced->m_name];
	}
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::memory_file(rynx::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	rynx::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	return attach_to_filetree(mountNode, rynx::make_shared<filetree::memoryfile_node>(mountNode->m_name));
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::memory_directory(rynx::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	rynx::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	return attach_to_filetree(mountNode, rynx::make_shared<filetree::memory_directory_node>(mountNode->m_name));
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::native_directory(rynx::string native_path, rynx::string virtual_path) {
	remove_backslashes_from_path(virtual_path);
	if (native_path.back() != '/')
		native_path += "/";

	rynx_assert(rynx::filesystem::native::directory_exists(native_path), "mounting native directory '%s' - it does not exist.", native_path.c_str());
	rynx::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	return attach_to_filetree(mountNode,rynx::make_shared<filetree::native_directory_node>(mountNode->m_name, native_path));
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::native_file(rynx::string native_path, rynx::string virtual_path) {
	remove_backslashes_from_path(native_path);
	if (native_path.back() == '/')
		native_path.pop_back();

	rynx_assert(rynx::filesystem::native::file_exists(native_path), "mounting native file '%s' - it does not exist.", native_path.c_str());
	rynx::shared_ptr<filetree::node> mountNode = vfs_tree_mount(std::move(virtual_path));
	return attach_to_filetree(mountNode, rynx::make_shared<filetree::native_file_node>(mountNode->m_name, native_path));
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::memory_directory_compressed(rynx::string virtual_path) {
	auto node = memory_directory(std::move(virtual_path));
	node->m_compress_files = true;
	return node;
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::native_directory_compressed(rynx::string native_path, rynx::string virtual_path) {
	auto node = native_directory(std::move(native_path), std::move(virtual_path));
	node->m_compress_files = true;
	return node;
}

namespace {
	// removes the first path token from given path string
	// returns the first path token as a separate string value
	rynx::string firstPathToken(rynx::string& path) {
		auto delim_pos = path.find_first_of('/');
		if (delim_pos != rynx::string::npos) {
			rynx::string token = path.substr(0, delim_pos);
			path = path.substr(delim_pos + 1);
			return token;
		}

		rynx::string token = path;
		path.clear();
		return token;
	}
}

rynx::shared_ptr<rynx::filesystem::filetree::node> rynx::filesystem::vfs::vfs_tree_mount(rynx::string virtual_path) const {
	rynx::shared_ptr<rynx::filesystem::filetree::node> currentNode(m_filetree_root);
	rynx::string currentTokenName;

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
			auto emptyNode = rynx::make_shared<rynx::filesystem::filetree::node>(currentTokenName);
			emptyNode->m_parent = currentNode;
			
			currentNode->m_children.insert(std::make_pair(currentTokenName, std::move(emptyNode)));
			currentNode = currentNode->m_children[currentTokenName];
		}

		if (virtual_path.empty())
			break;
	}

	return currentNode;
}

std::pair<rynx::shared_ptr<rynx::filesystem::filetree::node>, rynx::string> rynx::filesystem::vfs::vfs_tree_access(rynx::string virtual_path) const {
	rynx::shared_ptr<rynx::filesystem::filetree::node> currentNode = m_filetree_root;
	rynx::string currentTokenName;
	rynx::string remaining_path;
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