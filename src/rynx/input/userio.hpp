
#pragma once

#include <array>
#include <cstdlib>
#include <rynx/std/memory.hpp>
#include <rynx/std/string.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/input/key_types.hpp>
#include <rynx/input/midi_input.hpp>

struct GLFWwindow;

class Window;

namespace rynx {

	class scoped_input_inhibitor;

	class InputDLL input
	{
		friend class scoped_input_inhibitor;

		void onKeyEvent(int key, int scancode, int action, int mods);
		void onMouseButtonEvent(int key, int action, int mods);
		void onMouseScrollEvent(double xoffset, double yoffset);
		void onMouseMoveEvent(double xpos, double ypos);
		void onMouseEnterEvent(int entered);

		struct midi_event {
			uint8_t status;
			uint8_t key;
			uint8_t velocity;
			uint8_t padding;
		};

		std::array<uint8_t, 512> m_buttonStates;
		std::array<midi_event, 32> m_midiEventQueue;

		int32_t m_midiEventQueueWrite = 1;
		int32_t m_midiEventQueueRead = 0;

		bool queueMidiEvent(uint8_t status, uint8_t key, uint8_t velocity) {
			// if something other than note on/off, we don't care. just report success.
			if (status > 0x90)
				return true;
			
			/*
			if (status >= 240) // system message
				return;
			if (status == 176) // control change message
				return;
			if (status == 224) // pitch bend message
				return;
			if (status == 192) // program change message
				return;
			*/

			if (m_midiEventQueueWrite == m_midiEventQueueRead) {
				// queue is full, ignore events
				return false;
			}

			m_midiEventQueue[m_midiEventQueueWrite] = {status, key, velocity, uint8_t()};
			m_midiEventQueueWrite = (m_midiEventQueueWrite + 1) % m_midiEventQueue.size();
			return true;
		}

		void processMidiEventQueue() {
			while ((m_midiEventQueueRead + 1) % m_midiEventQueue.size() != m_midiEventQueueWrite) {
				m_midiEventQueueRead = (m_midiEventQueueRead + 1) % m_midiEventQueue.size();
				auto midi_event = m_midiEventQueue[m_midiEventQueueRead];
				if (midi_event.status == 0x90) {
					m_buttonStates[getMidiKeyCode(midi_event.key).id] |= KEY_PRESSED | KEY_DOWN;
				}
				else if (midi_event.status == 0x80) {
					m_buttonStates[getMidiKeyCode(midi_event.key).id] = KEY_RELEASED | KEY_CLICK;
				}
			}
		}

		vec3<float> m_mouseDelta;
		vec3<float> m_mousePosition;
		vec3<float> m_mousePosition_clickBegin;
		rynx::shared_ptr<Window> m_window;
		float m_mouseScroll = 0;
		bool m_mouseInScreen = true;

		enum {
			KEY_DOWN = 1,
			KEY_PRESSED = 2,
			KEY_REPEAT = 4,
			KEY_RELEASED = 8,
			KEY_CLICK = 16,
			KEY_CONSUMED = 32
		};

		struct inhibit_state {
			int32_t keyboard = 0;
			int32_t mouse = 0;
		};

		inhibit_state m_inhibited;

		int32_t isInhibited(rynx::key::physical key) const {
			int32_t keyboardInhibited = ((m_inhibited.keyboard > 0) * (key.id < getMouseKeyCode(0).id));
			int32_t mouseInhibited = ((m_inhibited.mouse > 0) * (key.id >= getMouseKeyCode(0).id));
			return keyboardInhibited | mouseInhibited;
		}

		int32_t isUnhinged(rynx::key::physical key) const {
			return isInhibited(key) ^ 1;
		}

		static bool isKeyValueInState(int8_t keyState, int8_t requestedState, bool ignoreConsumed);
		inline bool isKeyInState(rynx::key::physical key, int8_t requestedState, bool ignoreConsumed) const {
			return
				isUnhinged(key) *
				isKeyValueInState(m_buttonStates[key.id], requestedState, ignoreConsumed);
		}

	public:
		input(rynx::shared_ptr<Window> window);
		~input();

		// as long as you hold on to the scoped inhibitor, the inhibit holds.
		scoped_input_inhibitor inhibit_mouse_scoped();
		scoped_input_inhibitor inhibit_keyboard_scoped();
		scoped_input_inhibitor inhibit_mouse_and_keyboard_scoped();

		struct modifiers_t {
			
			modifiers_t(rynx::input* host) : m_host(host) {}

			bool shift_left();
			bool shift_right();

			bool alt_left();
			bool alt_right();

			bool ctrl_left();
			bool ctrl_right();

			bool shift() { return shift_left() || shift_right(); }
			bool alt() { return alt_left() || alt_right(); }
			bool ctrl() { return ctrl_left() || ctrl_right(); }
		
		private:
			rynx::input* m_host;
		};

		modifiers_t modifiers() { return { this }; }

		constexpr rynx::key::physical getMouseKeyCode(int mouseButton) const;
		constexpr rynx::key::physical getMidiKeyCode(int midiButton) const;


		template<class T>
		inline void getCursorPosition(T& t) const {
			t.x = m_mousePosition.x;
			t.y = m_mousePosition.y;
		}

		vec3<float> getCursorPosition() const
		{
			return m_mousePosition;
		}

		vec3<float> getCursorDelta() const
		{
			return m_mouseDelta;
		}

		bool isKeyClicked(rynx::key::physical key, bool ignoreConsumed = false) const;
		bool isKeyPressed(rynx::key::physical key, bool ignoreConsumed = false) const;
		bool isKeyDown(rynx::key::physical key, bool ignoreConsumed = false) const;
		bool isKeyRepeat(rynx::key::physical key, bool ignoreConsumed = false) const;
		bool isKeyReleased(rynx::key::physical key, bool ignoreConsumed = false) const;
		bool isKeyConsumed(rynx::key::physical key) const;
		void consume(rynx::key::physical key);

		float getMouseScroll() const;

		rynx::key::physical getAnyClicked();
		rynx::key::physical getAnyReleased();

		void update();

		rynx::io::midi::device listenToMidiDevice(int index);
		
		static void keyCallbackDummy(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseButtonCallbackDummy(GLFWwindow* window, int button, int action, int mods);
		static void scrollCallbackDummy(GLFWwindow* window, double xoffset, double yoffset);
		static void cursorPosCallbackDummy(GLFWwindow* window, double xpos, double ypos);
		static void cursorEnterCallbackDummy(GLFWwindow* window, int entered);
	};

	class scoped_input_inhibitor {
	public:
		scoped_input_inhibitor() {}
		scoped_input_inhibitor(input::inhibit_state* host, bool keyboard, bool mouse) : host(host), inhibit_mouse(mouse), inhibit_keyboard(keyboard) {
			if (inhibit_mouse) {
				++host->mouse;
			}
			if (inhibit_keyboard) {
				++host->keyboard;
			}
		}

		~scoped_input_inhibitor() {
			if (inhibit_mouse) {
				--host->mouse;
			}
			if (inhibit_keyboard) {
				--host->keyboard;
			}
		}

	private:
		input::inhibit_state* host = nullptr;
		bool inhibit_mouse = false;
		bool inhibit_keyboard = false;
	};
}