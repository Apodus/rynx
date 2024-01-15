
#pragma once

#include <utility>
#include <cstdint>
#include <cuchar>

namespace rynx {
	namespace key {
		// a logical key is a name for an action specified by the application. for example "MoveForward" could be a logical key.
		// a logical key is then mapped to some physical key. multiple logical keys can be mapped to the same physical key.
		// this means that the logical key can stay constant in the application even when rebinding keys. the action stays the same,
		// just the physical key that is bound to that logical action changes - and that happens under the hood. you do not need to keep track.
		struct logical {
			struct hash { size_t operator()(const rynx::key::logical& key) const { return key.action_id; } };
			bool operator == (const rynx::key::logical&) const = default;
			int32_t action_id = -1;
		};

		struct physical {
			struct hash { size_t operator()(const rynx::key::physical& key) const { return key.id; } };
			bool operator == (const rynx::key::physical&) const = default;
			explicit operator bool() const { return id > 0; }
			int32_t id = -1;
		};

		namespace codes {
			InputDLL rynx::key::physical arrow_left();
			InputDLL rynx::key::physical arrow_right();
			InputDLL rynx::key::physical arrow_up();
			InputDLL rynx::key::physical arrow_down();
			InputDLL rynx::key::physical space();
			InputDLL rynx::key::physical alt_left();
			InputDLL rynx::key::physical control_left();
			InputDLL rynx::key::physical alt_right();
			InputDLL rynx::key::physical control_right();
			InputDLL rynx::key::physical apostrophe();
			InputDLL rynx::key::physical backslash();
			InputDLL rynx::key::physical backspace();
			InputDLL rynx::key::physical capslock();
			InputDLL rynx::key::physical comma();
			InputDLL rynx::key::physical delete_key();
			InputDLL rynx::key::physical end();
			InputDLL rynx::key::physical enter();
			InputDLL rynx::key::physical equal();
			InputDLL rynx::key::physical escape();
			InputDLL rynx::key::physical grace_accent();
			InputDLL rynx::key::physical home();
			InputDLL rynx::key::physical insert();
			InputDLL rynx::key::physical keypad_0();
			InputDLL rynx::key::physical keypad_1();
			InputDLL rynx::key::physical keypad_2();
			InputDLL rynx::key::physical keypad_3();
			InputDLL rynx::key::physical keypad_4();
			InputDLL rynx::key::physical keypad_5();
			InputDLL rynx::key::physical keypad_6();
			InputDLL rynx::key::physical keypad_7();
			InputDLL rynx::key::physical keypad_8();
			InputDLL rynx::key::physical keypad_9();
			InputDLL rynx::key::physical keypad_add();
			InputDLL rynx::key::physical keypad_decimal();
			InputDLL rynx::key::physical keypad_divide();
			InputDLL rynx::key::physical keypad_enter();
			InputDLL rynx::key::physical keypad_equal();
			InputDLL rynx::key::physical keypad_multiply();
			InputDLL rynx::key::physical keypad_subtract();
			InputDLL rynx::key::physical bracket_left();
			InputDLL rynx::key::physical shift_left();
			InputDLL rynx::key::physical menu();
			InputDLL rynx::key::physical minus();
			InputDLL rynx::key::physical num_lock();
			InputDLL rynx::key::physical page_down();
			InputDLL rynx::key::physical page_up();
			InputDLL rynx::key::physical pause();
			InputDLL rynx::key::physical dot();
			InputDLL rynx::key::physical print_screen();
			InputDLL rynx::key::physical bracket_right();
			InputDLL rynx::key::physical shift_right();
			InputDLL rynx::key::physical scroll_lock();
			InputDLL rynx::key::physical semicolon();
			InputDLL rynx::key::physical slash();
			InputDLL rynx::key::physical tab();
		};

		static constexpr rynx::key::logical invalid_logical_key{ 0 };
		static constexpr rynx::key::physical invalid_physical_key{ 0 };
	}
}
