
#include <rynx/menu/Frame.hpp>

#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/mesh/mesh.hpp>


rynx::menu::Frame::Frame(
	Component* parent,
	GPUTextures& textures,
	const std::string& textureID,
	float edgeSize) : Component(parent, vec3<float>(), vec3<float>()) {
	m_backgroundMesh = std::make_unique<Mesh>();
	m_color = Color::WHITE;
	m_textureID = textureID;
	m_edgeSize = edgeSize;

	position_local({ 0, 0, 0 });
	scale_local({ 1, 1, 0 });

	initMesh(textures);
	buildMesh(0.5f, 0.5f);
	m_backgroundMesh->build();
}

void rynx::menu::Frame::initMesh(GPUTextures& textures) {
	floats4 limits = textures.textureLimits(m_textureID);

	float edgeUV_x = m_edgeSize * (limits.data[2] - limits[0]);
	float edgeUV_y = m_edgeSize * (limits.data[3] - limits[1]);

	float botCoordX = limits.data[0];
	float botCoordY = limits.data[1];
	float topCoordX = limits.data[2];
	float topCoordY = limits.data[3];

	m_backgroundMesh->texCoords.clear();
	m_backgroundMesh->putUVCoord(botCoordX, topCoordY);
	m_backgroundMesh->putUVCoord(botCoordX + edgeUV_x, topCoordY);
	m_backgroundMesh->putUVCoord(topCoordX - edgeUV_x, topCoordY);
	m_backgroundMesh->putUVCoord(topCoordX, topCoordY);

	m_backgroundMesh->putUVCoord(botCoordX, topCoordY - edgeUV_y);
	m_backgroundMesh->putUVCoord(botCoordX + edgeUV_x, topCoordY - edgeUV_y);
	m_backgroundMesh->putUVCoord(topCoordX - edgeUV_x, topCoordY - edgeUV_y);
	m_backgroundMesh->putUVCoord(topCoordX, topCoordY - edgeUV_y);

	m_backgroundMesh->putUVCoord(botCoordX, botCoordY + edgeUV_y);
	m_backgroundMesh->putUVCoord(botCoordX + edgeUV_x, botCoordY + edgeUV_y);
	m_backgroundMesh->putUVCoord(topCoordX - edgeUV_x, botCoordY + edgeUV_y);
	m_backgroundMesh->putUVCoord(topCoordX, botCoordY + edgeUV_y);

	m_backgroundMesh->putUVCoord(botCoordX, botCoordY);
	m_backgroundMesh->putUVCoord(botCoordX + edgeUV_x, botCoordY);
	m_backgroundMesh->putUVCoord(topCoordX - edgeUV_x, botCoordY);
	m_backgroundMesh->putUVCoord(topCoordX, botCoordY);

	m_backgroundMesh->indices.clear();
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			int index = x + 4 * y;
			m_backgroundMesh->putTriangleIndices(index, index + 5, index + 4);
			m_backgroundMesh->putTriangleIndices(index, index + 1, index + 5);
		}
	}
}

void rynx::menu::Frame::buildMesh(float size_x, float size_y) {
	float edgeSizeX = 0.05f / size_x;
	float edgeSizeY = 0.05f / size_y;
	
	edgeSizeY = (edgeSizeY > 0.5f) ? 0.5f : edgeSizeY;
	edgeSizeX = (edgeSizeX > 0.5f) ? 0.5f : edgeSizeX;

	m_backgroundMesh->vertices.clear();
	m_backgroundMesh->putVertex(-1, -1, 0);
	m_backgroundMesh->putVertex(-1 + edgeSizeX, -1, 0);
	m_backgroundMesh->putVertex(+1 - edgeSizeX, -1, 0);
	m_backgroundMesh->putVertex(+1, -1, 0);
	
	m_backgroundMesh->putVertex(-1, -1 + edgeSizeY, 0);
	m_backgroundMesh->putVertex(-1 + edgeSizeX, -1 + edgeSizeY, 0);
	m_backgroundMesh->putVertex(+1 - edgeSizeX, -1 + edgeSizeY, 0);
	m_backgroundMesh->putVertex(+1, -1 + edgeSizeY, 0);
	
	m_backgroundMesh->putVertex(-1, +1 - edgeSizeY, 0);
	m_backgroundMesh->putVertex(-1 + edgeSizeX, +1 - edgeSizeY, 0);
	m_backgroundMesh->putVertex(+1 - edgeSizeX, +1 - edgeSizeY, 0);
	m_backgroundMesh->putVertex(+1, +1 - edgeSizeY, 0);
	
	m_backgroundMesh->putVertex(-1, +1, 0);
	m_backgroundMesh->putVertex(-1 + edgeSizeX, +1, 0);
	m_backgroundMesh->putVertex(+1 - edgeSizeX, +1, 0);
	m_backgroundMesh->putVertex(+1, +1, 0);
}


void rynx::menu::Frame::update(float) {
	const auto& parentPosition = parent()->position_world();
	const auto& parentDimensions = parent()->scale_world();
	
	if ((m_prevScale - parentDimensions).lengthSquared() > 0.001f * parentDimensions.lengthSquared()) {
		m_prevScale = parentDimensions;
		buildMesh(parentDimensions.x, parentDimensions.y);
		m_backgroundMesh->rebuildVertexBuffer();
	}

	m_modelMatrix.discardSetTranslate(parentPosition.x, parentPosition.y, 0);
	m_modelMatrix.scale(parentDimensions.x * .5f, parentDimensions.y * .5f, 1);
}

void rynx::menu::Frame::draw(rynx::MeshRenderer& meshRenderer, TextRenderer&) const {
	meshRenderer.drawMesh(*m_backgroundMesh, m_modelMatrix, m_textureID, m_color);
}