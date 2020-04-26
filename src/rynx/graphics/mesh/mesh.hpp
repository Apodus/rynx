
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/math/vector.hpp>
#include <vector>

namespace rynx {
	class mesh {
	public:
		mesh();

		void putVertex(float x, float y, float z);
		void putVertex(rynx::vec3f v);
		
		void putUVCoord(float u, float v);
		
		void putNormal(float x, float y, float z);
		void putNormal(rynx::vec3f v);

		void putTriangleIndices(int i1, int i2, int i3);

		static_assert(sizeof(short) == 2, "oh crap");

		std::vector<float> vertices;
		std::vector<float> texCoords;
		std::vector<float> normals;
		std::vector<short> indices;

		int getVertexCount() const;
		int getIndexCount() const;

		void build();
		void rebuildTextureBuffer();
		void rebuildVertexBuffer();
		void rebuildNormalBuffer();
		void bind() const;

	private:
		/*
			Shaders must use following attribute layout for mesh data:
				layout(location = 0) in vec3 position;
				layout(location = 1) in vec2 texCoord;
				layout(location = 2) in vec3 normal;
		*/
		enum Attributes { POSITION, TEXCOORD, NORMAL };

		GLuint vao;

		GLuint vbo;
		GLuint tbo;
		GLuint nbo;
		GLuint ibo;
	};
}