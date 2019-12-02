
#include "mesh.hpp"
#include <rynx/graphics/opengl.hpp>
#include <rynx/system/assert.hpp>

Mesh::Mesh() {
	numVertices = 0;
	numIndices = 0;
}

void Mesh::putVertex(float x, float y, float z) {
	vertices.push_back(x);
	vertices.push_back(y);
	vertices.push_back(z);
	++numVertices;
}

void Mesh::putUVCoord(float u, float v) {
	texCoords.push_back(u);
	texCoords.push_back(v);
}

void Mesh::putTriangleIndices(int i1, int i2, int i3) {
	indices.push_back(static_cast<short>(i1));
	indices.push_back(static_cast<short>(i2));
	indices.push_back(static_cast<short>(i3));
	numIndices += 3;
}

void Mesh::putNormal(float x, float y, float z) {
	normals.push_back(x);
	normals.push_back(y);
	normals.push_back(z);
}

int Mesh::getVertexCount() const {
	return numVertices;
}

int Mesh::getIndexCount() const {
	return numIndices;
}

void Mesh::build() {

	auto verifyNoGlErrors = []() {
		auto error = glGetError();
		rynx_assert(error == GL_NO_ERROR, "oh shit");
		return error == GL_NO_ERROR;
	};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(POSITION);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texCoords.size(), &texCoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXCOORD);

	//glGenBuffers(1, &nbo);
	//glBindBuffer(GL_ARRAY_BUFFER, nbo);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), &normals[0], GL_STATIC_DRAW);
	//glVertexAttribPointer(NORMALS, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glEnableVertexAttribArray(NORMALS);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * indices.size(), &indices[0], GL_STATIC_DRAW);

	verifyNoGlErrors();
}

void Mesh::rebuildTextureBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * texCoords.size(), &texCoords[0]);
}

void Mesh::rebuildVertexBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), &vertices[0]);
}

void Mesh::bind() const {
	glBindVertexArray(vao);
}