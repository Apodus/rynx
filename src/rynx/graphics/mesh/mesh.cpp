
#include "mesh.hpp"
#include <rynx/graphics/internal/includes.hpp>
#include <rynx/system/assert.hpp>

rynx::mesh::mesh() {}

rynx::mesh::~mesh() {
	bind();

	if (vbo != ~0u) {
		glDeleteBuffers(1, &vbo);
	}
	
	if (tbo != ~0u) {
		glDeleteBuffers(1, &tbo);
	}

	if (nbo != ~0u) {
		glDeleteBuffers(1, &nbo);
	}

	if (ibo != ~0u) {
		glDeleteBuffers(1, &ibo);
	}

	if (vao != ~0u) {
		glDeleteVertexArrays(1, &vao);
	}

	auto verifyNoGlErrors = []() {
		auto error = glGetError();
		rynx_assert(error == GL_NO_ERROR, "oh shit");
		return error == GL_NO_ERROR;
	};

	rynx_assert(verifyNoGlErrors(), "gl error");
}

void rynx::mesh::putVertex(float x, float y, float z) {
	vertices.push_back(x);
	vertices.push_back(y);
	vertices.push_back(z);
}

void rynx::mesh::putUVCoord(float u, float v) {
	texCoords.push_back(u);
	texCoords.push_back(v);
}

void rynx::mesh::putTriangleIndices(int i1, int i2, int i3) {
	indices.push_back(static_cast<short>(i1));
	indices.push_back(static_cast<short>(i2));
	indices.push_back(static_cast<short>(i3));
}

void rynx::mesh::putNormal(float x, float y, float z) {
	normals.push_back(x);
	normals.push_back(y);
	normals.push_back(z);
}

void rynx::mesh::putVertex(rynx::vec3f v) {
	putVertex(v.x, v.y, v.z);
}

void rynx::mesh::putNormal(rynx::vec3f v) {
	putNormal(v.x, v.y, v.z);
}

int rynx::mesh::getVertexCount() const {
	return int(vertices.size() / 3);
}

int rynx::mesh::getIndexCount() const {
	return int(indices.size());
}

void rynx::mesh::build() {

	auto verifyNoGlErrors = []() {
		auto error = glGetError();
		rynx_assert(error == GL_NO_ERROR, "oh shit");
		return error == GL_NO_ERROR;
	};

	rynx_assert(normals.size() == vertices.size(), "normals vs. vertices size mismatch!");

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

	glGenBuffers(1, &nbo);
	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(NORMAL);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * indices.size(), &indices[0], GL_STATIC_DRAW);

	verifyNoGlErrors();
}

void rynx::mesh::rebuildTextureBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * texCoords.size(), &texCoords[0]);
}

void rynx::mesh::rebuildVertexBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * vertices.size(), &vertices[0]);
}

void rynx::mesh::rebuildNormalBuffer() {
	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * normals.size(), &normals[0]);
}

void rynx::mesh::bind() const {
	glBindVertexArray(vao);
}