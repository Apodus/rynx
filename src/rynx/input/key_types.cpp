#pragma once

#include <rynx/input/key_types.hpp>
#include <GLFW/glfw3.h>

namespace rynx {
	namespace key {
		namespace codes {
			rynx::key::physical arrow_left() { return { GLFW_KEY_LEFT }; }
			rynx::key::physical arrow_right() { return { GLFW_KEY_RIGHT }; }
			rynx::key::physical arrow_up() { return { GLFW_KEY_UP }; }
			rynx::key::physical arrow_down() { return { GLFW_KEY_DOWN }; }
			rynx::key::physical space() { return { GLFW_KEY_SPACE }; }
			rynx::key::physical alt_left() { return { GLFW_KEY_LEFT_ALT }; }
			rynx::key::physical control_left() { return { GLFW_KEY_LEFT_CONTROL }; }
			rynx::key::physical alt_right() { return { GLFW_KEY_RIGHT_ALT }; }
			rynx::key::physical control_right() { return { GLFW_KEY_RIGHT_CONTROL }; }
			rynx::key::physical apostrophe() { return { GLFW_KEY_APOSTROPHE }; }
			rynx::key::physical backslash() { return { GLFW_KEY_BACKSLASH }; }
			rynx::key::physical backspace() { return { GLFW_KEY_BACKSPACE }; }
			rynx::key::physical capslock() { return { GLFW_KEY_CAPS_LOCK }; }
			rynx::key::physical comma() { return { GLFW_KEY_COMMA }; }
			rynx::key::physical delete_key() { return { GLFW_KEY_DELETE }; }
			rynx::key::physical end() { return { GLFW_KEY_END }; }
			rynx::key::physical enter() { return { GLFW_KEY_ENTER }; }
			rynx::key::physical equal() { return { GLFW_KEY_EQUAL }; }
			rynx::key::physical escape() { return { GLFW_KEY_ESCAPE }; }
			rynx::key::physical grace_accent() { return { GLFW_KEY_GRAVE_ACCENT }; }
			rynx::key::physical home() { return { GLFW_KEY_HOME }; }
			rynx::key::physical insert() { return { GLFW_KEY_INSERT }; }
			rynx::key::physical keypad_0() { return { GLFW_KEY_KP_0 }; }
			rynx::key::physical keypad_1() { return { GLFW_KEY_KP_1 }; }
			rynx::key::physical keypad_2() { return { GLFW_KEY_KP_2 }; }
			rynx::key::physical keypad_3() { return { GLFW_KEY_KP_3 }; }
			rynx::key::physical keypad_4() { return { GLFW_KEY_KP_4 }; }
			rynx::key::physical keypad_5() { return { GLFW_KEY_KP_5 }; }
			rynx::key::physical keypad_6() { return { GLFW_KEY_KP_6 }; }
			rynx::key::physical keypad_7() { return { GLFW_KEY_KP_7 }; }
			rynx::key::physical keypad_8() { return { GLFW_KEY_KP_8 }; }
			rynx::key::physical keypad_9() { return { GLFW_KEY_KP_9 }; }
			rynx::key::physical keypad_add() { return { GLFW_KEY_KP_ADD }; }
			rynx::key::physical keypad_decimal() { return { GLFW_KEY_KP_DECIMAL }; }
			rynx::key::physical keypad_divide() { return { GLFW_KEY_KP_DIVIDE }; }
			rynx::key::physical keypad_enter() { return { GLFW_KEY_KP_ENTER }; }
			rynx::key::physical keypad_equal() { return { GLFW_KEY_KP_EQUAL }; }
			rynx::key::physical keypad_multiply() { return { GLFW_KEY_KP_MULTIPLY }; }
			rynx::key::physical keypad_subtract() { return { GLFW_KEY_KP_SUBTRACT }; }
			rynx::key::physical bracket_left() { return { GLFW_KEY_LEFT_BRACKET }; }
			rynx::key::physical shift_left() { return { GLFW_KEY_LEFT_SHIFT }; }
			rynx::key::physical menu() { return { GLFW_KEY_MENU }; }
			rynx::key::physical minus() { return { GLFW_KEY_MINUS }; }
			rynx::key::physical num_lock() { return { GLFW_KEY_NUM_LOCK }; }
			rynx::key::physical page_down() { return { GLFW_KEY_PAGE_DOWN }; }
			rynx::key::physical page_up() { return { GLFW_KEY_PAGE_UP }; }
			rynx::key::physical pause() { return { GLFW_KEY_PAUSE }; }
			rynx::key::physical dot() { return { GLFW_KEY_PERIOD }; }
			rynx::key::physical print_screen() { return { GLFW_KEY_PRINT_SCREEN }; }
			rynx::key::physical bracket_right() { return { GLFW_KEY_RIGHT_BRACKET }; }
			rynx::key::physical shift_right() { return { GLFW_KEY_RIGHT_SHIFT }; }
			rynx::key::physical scroll_lock() { return { GLFW_KEY_SCROLL_LOCK }; }
			rynx::key::physical semicolon() { return { GLFW_KEY_SEMICOLON }; }
			rynx::key::physical slash() { return { GLFW_KEY_SLASH }; }
			rynx::key::physical tab() { return { GLFW_KEY_TAB }; }
		}
	}
}