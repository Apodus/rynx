
#pragma once

#include <utility>
#include <cstdint>

namespace rynx {
	namespace key {
		// a logical key is a name for an action specified by the application. for example "MoveForward" could be a logical key.
		// a logical key is then mapped to some physical key. multiple logical keys can be mapped to the same physical key.
		// this means that the logical key can stay constant in the application even when rebinding keys. the action stays the same,
		// just the physical key that is bound to that logical action changes - and that happens under the hood. you do not need to keep track.
		struct logical {
			struct hash { size_t operator()(const rynx::key::logical& key) const { return std::hash<int32_t>()(key.action_id); } };
			bool operator == (const rynx::key::logical&) const = default;
			int32_t action_id = -1;
		};

		struct physical {
			struct hash { size_t operator()(const rynx::key::physical& key) const { return std::hash<int32_t>()(key.id); } };
			bool operator == (const rynx::key::physical&) const = default;
			operator bool() const { return id > 0; }
			int32_t id = -1;
		};

		namespace codes {
			rynx::key::physical arrow_left();
			rynx::key::physical arrow_right();
			rynx::key::physical arrow_up();
			rynx::key::physical arrow_down();
			rynx::key::physical space();
			rynx::key::physical alt_left();
			rynx::key::physical control_left();
			rynx::key::physical alt_right();
			rynx::key::physical control_right();
			rynx::key::physical apostrophe();
			rynx::key::physical backslash();
			rynx::key::physical backspace();
			rynx::key::physical capslock();
			rynx::key::physical comma();
			rynx::key::physical delete_key();
			rynx::key::physical end();
			rynx::key::physical enter();
			rynx::key::physical equal();
			rynx::key::physical escape();
			rynx::key::physical grace_accent();
			rynx::key::physical home();
			rynx::key::physical insert();
			rynx::key::physical keypad_0();
			rynx::key::physical keypad_1();
			rynx::key::physical keypad_2();
			rynx::key::physical keypad_3();
			rynx::key::physical keypad_4();
			rynx::key::physical keypad_5();
			rynx::key::physical keypad_6();
			rynx::key::physical keypad_7();
			rynx::key::physical keypad_8();
			rynx::key::physical keypad_9();
			rynx::key::physical keypad_add();
			rynx::key::physical keypad_decimal();
			rynx::key::physical keypad_divide();
			rynx::key::physical keypad_enter();
			rynx::key::physical keypad_equal();
			rynx::key::physical keypad_multiply();
			rynx::key::physical keypad_subtract();
			rynx::key::physical bracket_left();
			rynx::key::physical shift_left();
			rynx::key::physical menu();
			rynx::key::physical minus();
			rynx::key::physical num_lock();
			rynx::key::physical page_down();
			rynx::key::physical page_up();
			rynx::key::physical pause();
			rynx::key::physical dot();
			rynx::key::physical print_screen();
			rynx::key::physical bracket_right();
			rynx::key::physical shift_right();
			rynx::key::physical scroll_lock();
			rynx::key::physical semicolon();
			rynx::key::physical slash();
			rynx::key::physical tab();
		};

		static constexpr rynx::key::logical invalid_logical_key{ 0 };
		static constexpr rynx::key::physical invalid_physical_key{ 0 };
	}
}
