#pragma once

#include <rynx/input/userio.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/object_storage.hpp>

namespace rynx {
		class mapped_input {
		public:
			mapped_input(std::shared_ptr<rynx::input> physicalIO) {
				userIO = physicalIO;
				rebindAction(InvalidAction, InvalidAction);
			}

			// TODO: take modifiers into account (e.g. control + W != W)
			/*
			struct KeyCombination {
				int keyDown;
				int modifiers;
			};
			*/

			mapped_input& mouseWorldPosition(vec3<float> worldPos) {
				m_mouseWorldPos = worldPos;
				return *this;
			}

			vec3<float> mouseDelta() {
				return userIO->getCursorDelta();
			}

			vec3<float> mouseMenuPosition(float aspectRatio) {
				return mouseScreenPosition() * vec3<float>(1, 1.0f / aspectRatio, 1);
			}

			vec3<float> mouseWorldPosition() const {
				return m_mouseWorldPos;
			}

			vec3<float> mouseScreenPosition() const {
				return userIO->getCursorPosition();
			}

			int32_t getMouseKeyPhysical(int32_t mouseButton) const {
				return userIO->getMouseKeyCode(mouseButton);
			}

			// todo: should actions be bindable to scroll 'click' events? currently that is not possible.
			float getMouseScroll() const {
				return userIO->getMouseScroll();
			}

			int32_t generateAndBindGameKey(int32_t physicalKey, std::string actionName = "") {
				rebindAction(++gameKeyCounter, physicalKey);
				if (!actionName.empty())
					applicationKeyByName_map[actionName] = gameKeyCounter;
				return gameKeyCounter;
			}

			void rebindAction(int32_t applicationKey, int32_t physicalKey) {
				keyBindings[applicationKey] = physicalKey;
				reverseBindings[physicalKey] = applicationKey;
			}

			int32_t applicationKeyByName(const std::string& actionName) const {
				return applicationKeyByName_map.find(actionName)->second;
			}

			bool isKeyClicked(int32_t key) const {
				return userIO->isKeyClicked(keyBindings.find(key)->second);
			}

			bool isKeyPressed(int32_t key) const {
				return userIO->isKeyPressed(keyBindings.find(key)->second);
			}

			bool isKeyDown(int32_t key) const {
				return userIO->isKeyDown(keyBindings.find(key)->second);
			}

			bool isKeyRepeat(int32_t key) const {
				return userIO->isKeyRepeat(keyBindings.find(key)->second);
			}

			bool isKeyReleased(int32_t key) const {
				return userIO->isKeyReleased(keyBindings.find(key)->second);
			}

			bool isKeyConsumed(int32_t key) const {
				return userIO->isKeyConsumed(keyBindings.find(key)->second);
			}

			void consume(int32_t key) {
				return userIO->consume(keyBindings.find(key)->second);
			}

			// NOTE: getAny* functions are not guaranteed to return correct value in cases where you have bound several gameKey codes to the same physical key.
			int32_t getAnyClicked_GameKey() {
				auto clicked = userIO->getAnyClicked();
				if (clicked == 0)
					return 0;
				return reverseBindings.find(clicked)->second;
			}

			int32_t getAnyClicked_PhysicalKey() {
				return userIO->getAnyClicked();
			}

			int32_t getAnyReleased_GameKey() {
				auto released = userIO->getAnyReleased();
				if (released == 0)
					return 0;
				return reverseBindings.find(released)->second;
			}

			int32_t getAnyReleased_PhysicalKey() {
				return userIO->getAnyReleased();
			}

		private:
			static constexpr int32_t InvalidAction = 0;
			int32_t gameKeyCounter = 0;
			int32_t bindNextClicked = InvalidAction;
			std::shared_ptr<rynx::input> userIO;
			unordered_map<int32_t, int32_t> keyBindings;
			unordered_map<int32_t, int32_t> reverseBindings;
			unordered_map<std::string, int32_t> applicationKeyByName_map;

			vec3<float> m_mouseWorldPos; // TODO: is this an ok place to have this?
		};
}