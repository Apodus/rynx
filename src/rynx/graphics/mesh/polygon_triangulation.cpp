
#include <rynx/graphics/mesh/polygon_triangulation.hpp>
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/geometry/math.hpp>
#include <rynx/math/geometry/triangle.hpp>
#include <stdexcept>

namespace {
	struct TriangulationException {};
}

void rynx::polygon_triangulation::resetUnhandledMarkers() {
	unhandled.clear();
	for (unsigned i = 0; i < polygon->size(); ++i)
		unhandled.emplace_back(i);
}

void rynx::polygon_triangulation::resetForPostProcess() {
	triangles.clear();
	resetUnhandledMarkers();
}

rynx::vec3<float> rynx::polygon_triangulation::getVertexUnhandled(int i) {
	return polygon->vertex_position(unhandled[i]);
}

void rynx::polygon_triangulation::addTriangle(int earNode, int t1, int t2) {
	triangles.push_back(typename rynx::polygon_triangulation::triangle(unhandled[t1], unhandled[earNode], unhandled[t2]));
	for (unsigned i = earNode; i < unhandled.size() - 1; ++i)
		unhandled[i] = unhandled[i + 1];
	unhandled.pop_back();
}

rynx::unique_ptr<rynx::graphics::mesh> rynx::polygon_triangulation::buildMeshData(
	floats4 uvLimits,
	rynx::vec3<std::pair<float, float>> poly_extents,
	float uv_multiplier)
{
	rynx::unique_ptr<rynx::graphics::mesh> polyMesh = rynx::make_unique<rynx::graphics::mesh>();

	float uvHalfWidthX = (uvLimits.z - uvLimits.x) * 0.5f;
	float uvHalfHeightY = (uvLimits.w - uvLimits.y) * 0.5f;

	float uvCenterX = uvHalfWidthX + uvLimits.x;
	float uvCenterY = uvHalfHeightY + uvLimits.y;

	// build vertex buffer
	for (size_t i = 0; i < polygon->size(); ++i) {
		auto v = polygon->vertex_position(i);
		polyMesh->putVertex(v.x, v.y, 0.0f);
		
		float scaled_x = 2.0f * (v.x - poly_extents.x.first) / (poly_extents.x.second - poly_extents.x.first) - 1.0f;
		float scaled_y = 2.0f * (v.y - poly_extents.y.first) / (poly_extents.y.second - poly_extents.y.first) - 1.0f;

		rynx_assert(scaled_x <= +1.0f && scaled_y <= +1.0f, "");
		rynx_assert(scaled_x >= -1.0f && scaled_y >= -1.0f, "");

		float uvX = uvCenterX + scaled_x * uvHalfWidthX; // assumes [-1, +1] meshes
		float uvY = uvCenterY + scaled_y * uvHalfHeightY; // assumes [-1, +1] meshes
		// polyMesh->putUVCoord(uvX * uv_multiplier, uvY * uv_multiplier);
		polyMesh->putUVCoord(uvX, uvY);
	}


	// build normals buffer
	{
		auto push_normal = [this, &polyMesh](size_t prev, size_t current, size_t next) {
			auto ab = polygon->vertex_position(prev) - polygon->vertex_position(current);
			auto bc = polygon->vertex_position(current) - polygon->vertex_position(next);
			auto normal = (ab.normal2d().normalize() + bc.normal2d().normalize()).normalize();
			polyMesh->putNormal(normal.x, normal.y, normal.z);
		};

		push_normal(polygon->size() - 1, 0, 1);
		for (size_t i = 1; i < polygon->size() - 1; ++i) {
			push_normal(i - 1, i, i + 1);
		}
		push_normal(polygon->size() - 2, polygon->size() - 1, 0);
	}

	// build index buffer
	for (const auto& t : triangles) {
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

std::vector<rynx::polygon_triangulation::triangle> rynx::polygon_triangulation::make_triangle_indices(const rynx::polygon& polygon_) {
	makeTriangles(polygon_);
	return triangles;
}

rynx::triangles rynx::polygon_triangulation::make_triangles(const rynx::polygon& polygon_) {
	makeTriangles(polygon_);
	
	rynx::triangles tris;
	for (auto t : this->triangles) {
		tris.data().emplace_back(polygon_.vertex_position(t.a), polygon_.vertex_position(t.b), polygon_.vertex_position(t.c));
	}
	return tris;
}

rynx::unique_ptr<rynx::graphics::mesh> rynx::polygon_triangulation::make_mesh(const rynx::polygon& polygon_, floats4 uvLimits) {
	rynx::polygon copy = polygon_;
	copy.normalize();
	float uv_multiplier = 1.0f / copy.max_component_value();
	rynx::vec3<std::pair<float, float>> poly_extents = copy.extents();
	makeTriangles(copy);
	return buildMeshData(uvLimits, poly_extents, uv_multiplier);
}

rynx::unique_ptr<rynx::graphics::mesh> rynx::polygon_triangulation::make_boundary_mesh(const rynx::polygon& polygon_, float line_width, rynx::floats4 tex_coords) {
	
	rynx::unique_ptr<rynx::graphics::mesh> polyMesh = rynx::make_unique<rynx::graphics::mesh>();
	rynx::polygon copy = polygon_;
	copy.normalize();
	float radius = copy.radius();

	const float width = line_width / radius;

	auto get_vertex = [&copy](int index) {
		return copy.vertex_position((index + static_cast<int>(copy.size())) % copy.size());
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

	for (int i = 1; i < copy.size(); ++i) {
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

void rynx::polygon_triangulation::makeTriangles(const rynx::polygon& polygon_) {
	this->polygon = &polygon_;
	resetForPostProcess();

	try {
		triangulate();
	}
	catch (const TriangulationException&) {
		rynx::polygon copy = polygon_;
		polygon_editor editor(copy);
		editor.reverse();
		try {
			this->polygon = &copy;
			triangulate();
			this->polygon = nullptr;
		}
		catch (const TriangulationException&) {
			throw std::runtime_error("Broken polygon");
		}
	}
}