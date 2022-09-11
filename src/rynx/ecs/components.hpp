
#pragma once

#include <rynx/ecs/id.hpp>
#include <rynx/ecs/scenes.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/system/annotate.hpp>

namespace rynx::components {
	namespace transform {
		struct position {
			position() = default;
			position(rynx::vec3<float> pos, float angle = 0) : value(pos), angle(angle) {}
			rynx::vec3<float> value;
			float angle = 0;
		};

		struct scale {
			operator float() const {
				return value;
			}

			scale() : value(1.0f) {}
			scale(float v) : value(v) {}
			float ANNOTATE(">=0") value;
		};

		struct radius {
			radius() = default;
			radius(float r) : r(r) {}
			float r = 0;
		};

		struct ANNOTATE("transient") ANNOTATE("hidden") matrix : ecs_no_serialize_tag {
			bool operator == (const matrix & other) const { return m == other.m; }
			rynx::matrix4 m;
		};
	}
}

namespace rynx::components::scene {
	struct link {
		rynx::scene_id id;
	};

	struct ANNOTATE("transient") parent : public rynx::ecs_no_serialize_tag{
		parent() = default;
		parent(rynx::ecs_internal::id id) : entity(id) {}
		rynx::ecs_internal::id entity;
	};

	// need some way to figure out which entities belong to some scene,
	// so we don't serialize those entities when saving active scene.
	struct ANNOTATE("transient") children : public rynx::ecs_no_serialize_tag {
		rynx::entity_range_t entities;
	};

	// persistent ids are guaranteed to be unique in a given scene, and persistent over serialization.
	// the uniqueness guarantee for entities does not extend to sub-scenes.
	struct ANNOTATE("hidden") persistent_id {
		bool operator == (const persistent_id& other) const noexcept = default;
		int32_t value = 0;
	};

	struct ANNOTATE("hidden") ANNOTATE("transient") local_position : public ecs_no_serialize_tag {
		local_position() {}
		local_position(rynx::components::transform::position p) : pos(p) {}
		rynx::components::transform::position pos;
	};
}

#ifndef RYNX_CODEGEN
#include <rynx/ecs/components_reflection.hpp>
#include <rynx/ecs/components_serialization.hpp>
#endif