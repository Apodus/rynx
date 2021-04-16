
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/serialization.hpp>
#include <rynx/math/vector.hpp>
#include <vector>

namespace rynx {
	namespace graphics {

		struct mesh_id {
			bool operator == (const mesh_id& other) const = default;
			int64_t value;
		};

		class mesh {
		public:
			mesh();
			~mesh();

			void putVertex(float x, float y, float z);
			void putVertex(rynx::vec3f v);

			void putUVCoord(float u, float v);

			void putNormal(float x, float y, float z);
			void putNormal(rynx::vec3f v);

			void putTriangleIndices(int i1, int i2, int i3);

			static_assert(sizeof(short) == 2, "oh crap");

			int getVertexCount() const;
			int getIndexCount() const;

			void build();
			void rebuildTextureBuffer();
			void rebuildVertexBuffer();
			void rebuildNormalBuffer();
			void bind() const;

			// TODO: Move to some kind of material settings or something.
			float lighting_direction_bias = 0.0f; // light vs. geometry hit angle affects how strongly the light is applied. this is a constant offset to that value.
			float lighting_global_multiplier = 1.0f;

			std::vector<float> vertices;
			std::vector<float> texCoords;
			std::vector<float> normals;
			std::vector<short> indices;

			mesh_id id;
			std::string humanReadableId;

		private:
			/*
				Shaders must use following attribute layout for mesh data:
					layout(location = 0) in vec3 position;
					layout(location = 1) in vec2 texCoord;
					layout(location = 2) in vec3 normal;
			*/
			enum Attributes { POSITION, TEXCOORD, NORMAL };

			GLuint vao = ~0u;

			GLuint vbo = ~0u;
			GLuint tbo = ~0u;
			GLuint nbo = ~0u;
			GLuint ibo = ~0u;
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::graphics::mesh_id> {
			template<typename IOStream>
			void serialize(const rynx::graphics::mesh_id& meshId, IOStream& writer) {
				writer(meshId.value);
			}

			template<typename IOStream>
			void deserialize(rynx::graphics::mesh_id& meshId, IOStream& reader) {
				reader(meshId.value);
			}
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::graphics::mesh> {
			template<typename IOStream>
			void serialize(const rynx::graphics::mesh& mesh, IOStream& writer) {
				writer(mesh.humanReadableId);
				writer(mesh.id);
				writer(mesh.lighting_direction_bias);
				writer(mesh.lighting_global_multiplier);
				writer(mesh.vertices);
				writer(mesh.normals);
				writer(mesh.texCoords);
				writer(mesh.indices);
			}

			template<typename IOStream>
			void deserialize(rynx::graphics::mesh& mesh, IOStream& reader) {
				reader(mesh.humanReadableId);
				reader(mesh.id);
				reader(mesh.lighting_direction_bias);
				reader(mesh.lighting_global_multiplier);
				reader(mesh.vertices);
				reader(mesh.normals);
				reader(mesh.texCoords);
				reader(mesh.indices);
				mesh.bind();
				mesh.build();
			}
		};
	}
}

namespace std {
	template<>
	struct hash<rynx::graphics::mesh_id> {
		size_t operator()(const rynx::graphics::mesh_id& id) const {
			return id.value;
		}
	};
}