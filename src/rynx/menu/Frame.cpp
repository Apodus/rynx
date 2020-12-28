
#include <rynx/menu/Frame.hpp>

#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/mesh/mesh.hpp>


rynx::menu::Frame::Frame(
	rynx::graphics::GPUTextures& textures,
	const std::string& textureID,
	float textureEdgeSize,
	float meshEdgeSize) : Component(vec3<float>(), vec3<float>()) {
	m_backgroundMesh = std::make_unique<rynx::graphics::mesh>();
	m_color = Color::WHITE;
	m_edgeSize = textureEdgeSize;

	position_local({ 0, 0, 0 });
	scale_local({ 1, 1, 0 });

	buildMesh(meshEdgeSize, meshEdgeSize);
	initMesh(textures.textureLimits(textureID));
	m_backgroundMesh->build();
	m_backgroundMesh->texture_name = textureID;
}

void rynx::menu::Frame::initMesh(floats4 limits) {
	float edgeUV_x = m_edgeSize * (limits.z - limits.x);
	float edgeUV_y = m_edgeSize * (limits.w - limits.y);

	float botCoordX = limits.x;
	float botCoordY = limits.y;
	float topCoordX = limits.z;
	float topCoordY = limits.w;

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

	m_backgroundMesh->normals = m_backgroundMesh->vertices;
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
	
	if ((m_prevScale - parentDimensions).length_squared() > 0.001f * parentDimensions.length_squared()) {
		m_prevScale = parentDimensions;
		buildMesh(parentDimensions.x, parentDimensions.y);
		m_backgroundMesh->rebuildVertexBuffer();
	}

	m_modelMatrix.discardSetTranslate(parentPosition.x, parentPosition.y, 0);
	m_modelMatrix.scale(parentDimensions.x * .5f, parentDimensions.y * .5f, 1);
}

void rynx::menu::Frame::draw(rynx::graphics::renderer& renderer) const {
	renderer.drawMesh(*m_backgroundMesh, m_modelMatrix, m_color);
}