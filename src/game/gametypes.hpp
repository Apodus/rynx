#pragma once

#include <rynx/tech/object_storage.hpp>

namespace game
{
	// TODO: this is a fast hack. no clue if this is a good way to express this.
	struct LocalPlayerID {
		LocalPlayerID(uint32_t id) : entityId(id) {}
		virtual ~LocalPlayerID() {}
		uint32_t entityId;
	};

	struct hero {}; // tag for player controlled entity.
	struct dead {}; // tag for hp <= 0. because damage source can't know how to handle death of target.

	struct health {
		float maxHp = 100;
		float currentHp = 100;
	};
}
