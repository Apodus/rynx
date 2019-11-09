
#pragma once

#include <rynx/graphics/mesh/polygon.hpp>
#include <rynx/graphics/mesh/polygonEditor.hpp>
#include <rynx/graphics/mesh/math.hpp>
#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/math/math.hpp>

// Todo: Get rid of vec3 assumption

namespace Shape {
	inline Polygon<vec3<float>> makeAAOval(float radius, int vertices, float x_scale, float y_scale) {
		Polygon<vec3<float>> shape;
		PolygonEditor<vec3<float>> editor(shape);
		for (int i = 0; i < vertices; ++i) {
			float angle = float(-i * 2) * math::PI_float / vertices;
			editor.insertVertex(
				vec3<float>(
					x_scale * radius * math::sin_approx(angle),
					y_scale * radius * math::cos_approx(angle),
					0.0f
				)
			);
		}
		return shape;
	}

	inline Polygon<vec3<float>> makeCircle(float radius, int vertices) {
		return makeAAOval(radius, vertices, 1, 1);
	}

	inline Polygon<vec3<float>> makeTriangle(const float& radius) {
		return makeCircle(radius, 3);
	}

	inline Polygon<vec3<float>> makeRectangle(float width, float height, int widthDivs = 1, int heightDivs = 1) {
		Polygon<vec3<float>> shape;
		PolygonEditor<vec3<float>> editor(shape);

		auto addLine = [&](const vec3<float>& start, const vec3<float>& end, int divs) {
			rynx_assert(divs > 0, "zero segments is not an option");
			for (int i = 0; i < divs; ++i) {
				editor.insertVertex(start * float(divs - i) / float(divs) + end * float(i) / float(divs));
			}
		};

		auto topLeft = vec3<float>(-width / float(2), +height / float(2), float(0));
		auto botLeft = vec3<float>(-width / float(2), -height / float(2), float(0));
		auto botRight = vec3<float>(+width / float(2), -height / float(2), float(0));
		auto topRight = vec3<float>(+width / float(2), +height / float(2), float(0));

		addLine(topLeft, botLeft, heightDivs);
		addLine(botLeft, botRight, widthDivs);
		addLine(botRight, topRight, heightDivs);
		addLine(topRight, topLeft, widthDivs);

		return shape;
	}

	inline Polygon<vec3<float>> makeBox(float edgeLength) {
		return makeRectangle(edgeLength, edgeLength);
	}
}

