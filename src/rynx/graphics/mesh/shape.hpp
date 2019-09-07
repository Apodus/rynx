
#pragma once

#include <rynx/graphics/mesh/polygon.hpp>
#include <rynx/graphics/mesh/polygonEditor.hpp>
#include <rynx/graphics/mesh/math.hpp>
#include <rynx/tech/math/vector.hpp>

// Todo: Get rid of vec3 assumption

namespace Shape {
	template<class T>
	inline Polygon<vec3<T>> makeAAOval(const T& radius, int vertices, const T& x_scale, const T& y_scale) {
		Polygon<vec3<T>> shape;
		PolygonEditor<vec3<T>> editor(shape);
		for (int i = 0; i < vertices; ++i) {
			T angle = T(-i * 2) * math::PI<T>() / T(vertices);
			editor.insertVertex(
				vec3<T>(
					x_scale * radius * math::sin(angle),
					y_scale * radius * math::cos(angle),
					T(0)
					)
			);
		}
		return shape;
	}

	template<class T>
	inline Polygon<vec3<T>> makeCircle(const T& radius, int vertices) {
		return makeAAOval(radius, vertices, T(1), T(1));
	}

	template<class T>
	inline Polygon<vec3<T>> makeTriangle(const T& radius) {
		return makeCircle(radius, 3);
	}

	template<class T>
	inline Polygon<vec3<T>> makeRectangle(const T& width, const T& height, int widthDivs = 1, int heightDivs = 1) {
		Polygon<vec3<T>> shape;
		PolygonEditor<vec3<T>> editor(shape);

		auto addLine = [&](const vec3<T>& start, const vec3<T>& end, int divs) {
			rynx_assert(divs > 0, "zero segments is not an option");
			for (int i = 0; i < divs; ++i) {
				editor.insertVertex(start * T(divs - i) / T(divs) + end * T(i) / T(divs));
			}
		};

		auto topLeft = vec3<T>(-width / T(2), height / T(2), T(0));
		auto botLeft = vec3<T>(-width / T(2), -height / T(2), T(0));
		auto botRight = vec3<T>(+width / T(2), -height / T(2), T(0));
		auto topRight = vec3<T>(+width / T(2), +height / T(2), T(0));

		addLine(topLeft, botLeft, heightDivs);
		addLine(botLeft, botRight, widthDivs);
		addLine(botRight, topRight, heightDivs);
		addLine(topRight, topLeft, widthDivs);

		return shape;
	}

	template<class T>
	inline Polygon<vec3<T>> makeBox(const T& edgeLength) {
		return makeRectangle(edgeLength, edgeLength);
	}
}

