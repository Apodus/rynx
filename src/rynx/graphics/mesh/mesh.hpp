
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <vector>

class Mesh {
public:
	Mesh();

	void putVertex(float x, float y, float z);
	void putUVCoord(float u, float v);
	void putNormal(float x, float y, float z);
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
	void bind() const;

private:
	/*
		Shaders must use following attribute layout:
			layout(location = 0) in vec3 position;
			layout(location = 1) in vec2 texCoord;
			layout(location = 2) in mat4 model;
	*/
	enum Attributes { POSITION, TEXCOORD, MODEL_MATRIX };

	GLuint vao;
	GLuint vbo, tbo, nbo, ibo;	// vertices, texcoords, normals, indices

	int numVertices;
	int numIndices;
};