
#pragma once

#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/geometry/math.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>

// Todo: Get rid of vec3 assumption

namespace rynx {
	namespace Shape {
		inline polygon makeAAOval(float radius, int vertices, float x_scale, float y_scale) {
			polygon shape;
			polygon_editor editor(shape);
			for (int i = 0; i < vertices; ++i) {
				float angle = float(-i * 2) * math::pi / vertices;
				editor.insertVertex(
					vec3<float>(
						x_scale * radius * std::sin(angle),
						y_scale * radius * std::cos(angle),
						0.0f
					)
				);
			}
			return shape;
		}

		inline polygon makeCircle(float radius, int vertices) {
			return makeAAOval(radius, vertices, 1, 1);
		}

		inline polygon makeSphere(float radius, int vertices) {
			return makeAAOval(radius, vertices, 1, 1);
		}

		inline polygon makeTriangle(const float& radius) {
			return makeCircle(radius, 3);
		}

		inline polygon makeRectangle(float width, float height, int widthDivs = 1, int heightDivs = 1) {
			polygon shape;
			polygon_editor editor(shape);

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

		inline polygon makeBox(float edgeLength) {
			return makeRectangle(edgeLength, edgeLength);
		}
	}
}
