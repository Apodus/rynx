
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/std/string.hpp>
#include <rynx/graphics/mesh/id.hpp>
#include <rynx/tech/serialization_declares.hpp>
#include <rynx/math/vector.hpp>
#include <vector>

namespace rynx {
	namespace graphics {

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
			void scale(float v) {
				for (auto& vertexfloat : vertices) {
					vertexfloat *= v;
				}
				rebuildVertexBuffer();
			}
			void rebuildTextureBuffer();
			void rebuildVertexBuffer();
			void rebuildNormalBuffer();
			void bind() const;

			void set_transient() { m_is_transient = true; }
			bool is_transient() { return m_is_transient; }

			void set_category(rynx::string category) { m_category = category; }
			const rynx::string& get_category() const { return m_category; }

			// TODO: Move to some kind of material settings or something.
			float lighting_direction_bias = 0.0f; // light vs. geometry hit angle affects how strongly the light is applied. this is a constant offset to that value.
			float lighting_global_multiplier = 1.0f;

			std::vector<float> vertices;
			std::vector<float> texCoords;
			std::vector<float> normals;
			std::vector<short> indices;

			mesh_id id;
			rynx::string humanReadableId;
			rynx::string m_category;

			enum Mode {
				Triangles = 0,
				LineStrip = 1
			};

			float lineWidth = 2.0f;

			void setModeLineStrip() { mode &= ~255; mode |= Mode::LineStrip; }
			void setModeTriangles() { mode &= ~255; mode |= Mode::Triangles; }

			bool isModeLineStrip() const { return (mode & 15) == Mode::LineStrip; }
			bool isModeTriangles() const { return (mode & 15) == Mode::Triangles; }

			// transient meshes do not get serialized by mesh collection.
			bool m_is_transient = false;

		private:
			int32_t mode = 0;

			template<typename T> friend struct rynx::serialization::Serialize;

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
		template<> struct Serialize<rynx::graphics::mesh> {
			template<typename IOStream>
			void serialize(const rynx::graphics::mesh& mesh, IOStream& writer) {
				rynx::serialize(mesh.humanReadableId, writer);
				rynx::serialize(mesh.id, writer);
				rynx::serialize(mesh.lighting_direction_bias, writer);
				rynx::serialize(mesh.lighting_global_multiplier, writer);
				rynx::serialize(mesh.vertices, writer);
				rynx::serialize(mesh.normals, writer);
				rynx::serialize(mesh.texCoords, writer);
				rynx::serialize(mesh.indices, writer);
				rynx::serialize(mesh.mode, writer);
				rynx::serialize(mesh.lineWidth, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::graphics::mesh& mesh, IOStream& reader) {
				rynx::deserialize(mesh.humanReadableId, reader);
				rynx::deserialize(mesh.id, reader);
				rynx::deserialize(mesh.lighting_direction_bias, reader);
				rynx::deserialize(mesh.lighting_global_multiplier, reader);
				rynx::deserialize(mesh.vertices, reader);
				rynx::deserialize(mesh.normals, reader);
				rynx::deserialize(mesh.texCoords, reader);
				rynx::deserialize(mesh.indices, reader);
				rynx::deserialize(mesh.mode, reader);
				rynx::deserialize(mesh.lineWidth, reader);
				mesh.build();
			}
		};
	}
}
