
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/tech/serialization.hpp>

namespace rynx {
	namespace filesystem {
		class vfs;
	}

	struct scene_id {
		uint64_t m_random_1 = 0;
		uint64_t m_random_2 = 0;
		
		static scene_id generate();
		operator bool() const;
		bool operator < (const scene_id& other) const;
		operator rynx::string() const;

		struct hash {
			size_t operator()(const scene_id& id) const {
				return id.m_random_1 ^ id.m_random_2;
			}
		};
	};

	struct scene_info {
		rynx::scene_id id;
		rynx::string ui_path; // intended for UI use. user can set & modify the logical path and organize the scenes in editor this way, with no need to move the files on disk.
		rynx::string name; // scene name :)
	};

	namespace serialization {
		template<> struct Serialize<rynx::scene_id> {
			template<typename IOStream>
			void serialize(const rynx::scene_id& s, IOStream& writer) {
				rynx::serialize(s.m_random_1, writer);
				rynx::serialize(s.m_random_2, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::scene_id& s, IOStream& reader) {
				rynx::deserialize(s.m_random_1, reader);
				rynx::deserialize(s.m_random_2, reader);
			}
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::scene_info> {
			template<typename IOStream>
			void serialize(const rynx::scene_info& s, IOStream& writer) {
				rynx::serialize(s.id, writer);
				rynx::serialize(s.name, writer);
				rynx::serialize(s.ui_path, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::scene_info& s, IOStream& reader) {
				rynx::deserialize(s.id, reader);
				rynx::deserialize(s.name, reader);
				rynx::deserialize(s.ui_path, reader);
			}
		};
	}

	class scenes {
	public:
		const uint64_t serialized_scene_marker = 0x1234567812345678ull;
		
		template<typename IOStream>
		std::pair<bool, rynx::scene_info> read_definition(IOStream& stream) const {
			auto id = rynx::deserialize<uint64_t>(stream);
			if (id != serialized_scene_marker) {
				return { false, {} };
			}

			auto info = rynx::deserialize<scene_info>(stream);
			return { true, info };
		}

		template<typename IOStream>
		std::pair<bool, std::vector<char>> read_payload(IOStream& stream) const {
			if (read_definition(stream).first) {
				return { true, rynx::deserialize<std::vector<char>>(stream) };
			}
			return {};
		}

		void scan_directory(rynx::filesystem::vfs& fs, const rynx::string& logical_path);

		std::vector<char> get(rynx::filesystem::vfs& fs, rynx::scene_id id) const;
		std::vector<char> get(rynx::filesystem::vfs& fs, rynx::string filepath) const;
		
		void save_scene(
			rynx::filesystem::vfs& fs,
			rynx::serialization::vector_writer& serialized_scene,
			rynx::string ui_path,
			rynx::string scene_name,
			rynx::string filepath
		);

		rynx::scene_info& filepath_to_info(const rynx::string& path);
		const rynx::scene_info& filepath_to_info(const rynx::string& path) const;

		std::vector<std::pair<rynx::string, rynx::scene_id>> list_scenes() const;

	private:
		void internal_update(rynx::scene_info& info, rynx::string filepath);

		rynx::unordered_map<rynx::string, rynx::scene_info> m_filepath_to_info;
		rynx::unordered_map<rynx::scene_id, rynx::string, rynx::scene_id::hash> m_filepaths;
		rynx::unordered_map<rynx::scene_id, rynx::scene_info, rynx::scene_id::hash> m_infos;
	};
}