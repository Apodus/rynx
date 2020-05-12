
#include <rynx/math/geometry/polygon_triangulation.hpp>
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/geometry/math.hpp>

#include <stdexcept>

namespace {
	struct TriangulationException {};
}

std::vector<typename rynx::polygon::triangle>& rynx::polygon_triangulation::getTriangles() {
	return polygon->triangles;
}

void rynx::polygon_triangulation::resetUnhandledMarkers() {
	unhandled.clear();
	for (unsigned i = 0; i < polygon->vertices.size(); ++i)
		unhandled.emplace_back(i);
}

void rynx::polygon_triangulation::resetForPostProcess() {
	polygon->triangles.clear();
	resetUnhandledMarkers();
}

rynx::vec3<float>& rynx::polygon_triangulation::getVertexUnhandled(int i) {
	return polygon->vertices[unhandled[i]];
}

void rynx::polygon_triangulation::addTriangle(int earNode, int t1, int t2) {
	polygon->triangles.push_back(typename rynx::polygon::triangle(unhandled[t1], unhandled[earNode], unhandled[t2]));
	for (unsigned i = earNode; i < unhandled.size() - 1; ++i)
		unhandled[i] = unhandled[i + 1];
	unhandled.pop_back();
}

std::unique_ptr<rynx::mesh> rynx::polygon_triangulation::buildMeshData(floats4 uvLimits) {
	std::unique_ptr<rynx::mesh> polyMesh = std::make_unique<rynx::mesh>();

	float uvHalfWidthX = (uvLimits[2] - uvLimits[0]) * 0.5f;
	float uvHalfHeightY = (uvLimits[3] - uvLimits[1]) * 0.5f;

	float uvCenterX = uvHalfWidthX + uvLimits[0];
	float uvCenterY = uvHalfHeightY + uvLimits[1];

	// build vertex buffer
	for (rynx::vec3<float>& v : polygon->vertices) {
		polyMesh->putVertex(float(v.x), float(v.y), 0.0f);
		float uvX = uvCenterX + static_cast<float>(v.x) * uvHalfWidthX; // assumes [-1, +1] meshes
		float uvY = uvCenterY + static_cast<float>(v.y) * uvHalfHeightY; // assumes [-1, +1] meshes
		polyMesh->putUVCoord(uvX, uvY);
	}


	// build normals buffer
	{
		auto push_normal = [this, &polyMesh](size_t prev, size_t current, size_t next) {
			auto ab = polygon->vertices[prev] - polygon->vertices[current];
			auto bc = polygon->vertices[current] - polygon->vertices[next];
			auto normal = (ab.normal2d().normalize() + bc.normal2d().normalize()).normalize();
			polyMesh->putNormal(normal.x, normal.y, normal.z);
		};

		push_normal(polygon->vertices.size() - 1, 0, 1);
		for (size_t i = 1; i < polygon->vertices.size() - 1; ++i) {
			push_normal(i - 1, i, i + 1);
		}
		push_normal(polygon->vertices.size() - 2, polygon->vertices.size() - 1, 0);
	}

	// build index buffer
	for (const typename rynx::polygon::triangle& t : getTriangles()) {
		polyMesh->putTriangleIndices(t.a, t.b, t.c);
	}

	return std::move(polyMesh);
}

bool rynx::polygon_triangulation::isEar(int t1, int t2, int t3) {
	for (int i = 0; i < int(unhandled.size()); ++i) {
		if (i == t1 || i == t2 || i == t3)
			continue;
		if (math::pointInTriangle(getVertexUnhandled(i), getVertexUnhandled(t1), getVertexUnhandled(t2), getVertexUnhandled(t3))) {
			return false;
		}
	}
	return true;
}

bool rynx::polygon_triangulation::triangulateOneStep() {

	// Note: If there is enough love in the polygon, we are done.
	if (unhandled.size() < 3) {
		return true;
	}

	for (unsigned i = 0; i < unhandled.size() - 2; ++i) {
		if (math::pointLeftOfLine(getVertexUnhandled(i + 1), getVertexUnhandled(i), getVertexUnhandled(i + 2))) {
		}
		else {
			if (isEar(i + 1, i, i + 2)) {
				addTriangle(i + 1, i, i + 2);
				return false;
			}
		}
	}

	int k = static_cast<int>(unhandled.size());
	if (math::pointLeftOfLine(getVertexUnhandled(k - 1), getVertexUnhandled(k - 2), getVertexUnhandled(0))) {

	}
	else {
		if (isEar(k - 1, k - 2, 0)) {
			addTriangle(k - 1, k - 2, 0);
			return false;
		}
	}

	if (math::pointLeftOfLine(getVertexUnhandled(0), getVertexUnhandled(k - 1), getVertexUnhandled(1))) {

	}
	else {
		if (isEar(0, k - 1, 1)) {
			addTriangle(0, k - 1, 1);
			return false;
		}
	}

	throw TriangulationException();
}

void rynx::polygon_triangulation::triangulate() {
	while (!triangulateOneStep());
}

rynx::polygon_triangulation::polygon_triangulation() {
}

std::unique_ptr<rynx::mesh> rynx::polygon_triangulation::triangulate(const rynx::polygon& polygon_, floats4 uvLimits) {
	rynx::polygon copy = polygon_;
	copy.normalize();
	makeTriangles(copy);
	return buildMeshData(uvLimits);
}

std::unique_ptr<rynx::mesh> rynx::polygon_triangulation::generate_polygon_boundary(const rynx::polygon& polygon_, rynx::floats4 tex_coords) {
	
	std::unique_ptr<rynx::mesh> polyMesh = std::make_unique<rynx::mesh>();
	rynx::polygon copy = polygon_;
	float radius = copy.normalize();

	auto boundary = copy.vertices;
	const float width = 1.0f / radius;

	auto get_vertex = [&boundary](int index) {
		index += static_cast<int>(boundary.size());
		index = index % boundary.size();
		return boundary[index];
	};

	auto a = get_vertex(-1);
	auto b = get_vertex(0);
	auto c = get_vertex(+1);

	auto aa = a - b;
	auto bb = b - c;
	
	vec3f b_normal = (aa.normal2d().normalize() + bb.normal2d().normalize()).normalize();
	vec3f p1 = b + b_normal * width;
	vec3f p2 = b - b_normal * width;

	polyMesh->putVertex(p1);
	polyMesh->putVertex(p2);

	polyMesh->putNormal(b_normal);
	polyMesh->putNormal(-b_normal);

	polyMesh->putUVCoord(tex_coords.x, tex_coords.y);
	polyMesh->putUVCoord(tex_coords.z, tex_coords.w);

	for (int i = 1; i < boundary.size(); ++i) {
		a = get_vertex(i - 1);
		b = get_vertex(i + 0);
		c = get_vertex(i + 1);

		aa = a - b;
		bb = b - c;

		b_normal = (aa.normal2d().normalize() + bb.normal2d().normalize()).normalize();
		p1 = b + b_normal * width;
		p2 = b - b_normal * width;

		polyMesh->putVertex(p1);
		polyMesh->putVertex(p2);

		polyMesh->putNormal(b_normal);
		polyMesh->putNormal(-b_normal);

		polyMesh->putUVCoord(tex_coords.x, tex_coords.y);
		polyMesh->putUVCoord(tex_coords.z, tex_coords.w);

		polyMesh->putTriangleIndices(2 * i, 2 * i - 1, 2 * i - 2);
		polyMesh->putTriangleIndices(2 * i - 1, 2 * i, 2 * i + 1);
	}

	int numVerts = polyMesh->getVertexCount();
	polyMesh->putTriangleIndices(1, 0, numVerts - 1);
	polyMesh->putTriangleIndices(0, numVerts - 1, numVerts - 2);
	
	return polyMesh;
}

void rynx::polygon_triangulation::makeTriangles(rynx::polygon& polygon_) {
	polygon = &polygon_;
	resetForPostProcess();

	try {
		triangulate();
	}
	catch (const TriangulationException&) {
		polygon_editor editor(polygon_);
		editor.reverse();
		try {
			triangulate();
		}
		catch (const TriangulationException&) {
			throw std::runtime_error("Broken polygon");
		}
	}
}