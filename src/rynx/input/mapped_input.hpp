#pragma once

#include <rynx/input/userio.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/object_storage.hpp>

namespace rynx {
	class ray;
	class camera;

	class mapped_input {
	public:
		mapped_input(std::shared_ptr<rynx::input> physicalIO) {
			userIO = physicalIO;
			rebindAction(rynx::key::invalid_logical_key, rynx::key::invalid_physical_key);
		}

		// TODO: take modifiers into account (e.g. control + W != W)
		/*
		struct KeyCombination {
			int keyDown;
			int modifiers;
		};
		*/

		auto inhibit_mouse_scoped() { return this->userIO->inhibit_mouse_scoped(); }
		auto inhibit_keyboard_scoped() { return this->userIO->inhibit_keyboard_scoped(); }
		auto inhibit_mouse_and_keyboard_scoped() { return this->userIO->inhibit_mouse_and_keyboard_scoped(); }
		// TODO: support inhibit of logical keys?


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

		rynx::ray mouseRay(rynx::camera& cam);

		vec3<float> mouseScreenPosition() const {
			return userIO->getCursorPosition();
		}

		rynx::key::physical getMouseKeyPhysical(int32_t mouseButton) const {
			return userIO->getMouseKeyCode(mouseButton);
		}

		// todo: should actions be bindable to scroll 'click' events? currently that is not possible.
		float getMouseScroll() const {
			return userIO->getMouseScroll();
		}

		rynx::key::logical generateAndBindGameKey(rynx::key::physical physicalKey, std::string actionName = "") {
			rebindAction(rynx::key::logical{ ++gameKeyCounter }, physicalKey);
			if (!actionName.empty())
				applicationKeyByName_map[actionName] = rynx::key::logical{ gameKeyCounter };
			return { gameKeyCounter };
		}

		rynx::key::logical generateAndBindGameKey(char8_t physicalKey, std::string actionName = "") {
			return generateAndBindGameKey(rynx::key::physical{ int32_t(physicalKey) }, std::move(actionName));
		}

		void rebindAction(rynx::key::logical applicationKey, rynx::key::physical physicalKey) {
			auto original = keyBindings.find(applicationKey);
			if (original != keyBindings.end()) {
				auto original_phys = original->second;
				auto& boundGameKeys = reverseBindings[original_phys];
				auto it = std::find(boundGameKeys.begin(), boundGameKeys.end(), applicationKey);
				rynx_assert(it != boundGameKeys.end(), "key was supposed to be bound?");
				boundGameKeys.erase(it);
			}
			keyBindings[applicationKey] = physicalKey;
			reverseBindings[physicalKey].emplace_back(applicationKey);
		}

		rynx::key::logical applicationKeyByName(const std::string& actionName) const {
			return applicationKeyByName_map.find(actionName)->second;
		}

		bool isKeyClicked(rynx::key::logical key, bool ignoreConsumed = false) const {
			return userIO->isKeyClicked(keyBindings.find(key)->second, ignoreConsumed);
		}
		
		bool isKeyClicked(rynx::key::physical key, bool ignoreConsumed = false) const {
			return userIO->isKeyClicked(key, ignoreConsumed);
		}

		bool isKeyPressed(rynx::key::logical key, bool ignoreConsumed = false) const {
			return userIO->isKeyPressed(keyBindings.find(key)->second, ignoreConsumed);
		}

		bool isKeyPressed(rynx::key::physical key, bool ignoreConsumed = false) const {
			return userIO->isKeyPressed(key, ignoreConsumed);
		}

		bool isKeyDown(rynx::key::logical key, bool ignoreConsumed = false) const {
			return userIO->isKeyDown(keyBindings.find(key)->second, ignoreConsumed);
		}
		
		bool isKeyDown(rynx::key::physical key, bool ignoreConsumed = false) const {
			return userIO->isKeyDown(key, ignoreConsumed);
		}

		bool isKeyRepeat(rynx::key::logical key, bool ignoreConsumed = false) const {
			return userIO->isKeyRepeat(keyBindings.find(key)->second, ignoreConsumed);
		}

		bool isKeyRepeat(rynx::key::physical key, bool ignoreConsumed = false) const {
			return userIO->isKeyRepeat(key, ignoreConsumed);
		}

		bool isKeyReleased(rynx::key::logical key, bool ignoreConsumed = false) const {
			return userIO->isKeyReleased(keyBindings.find(key)->second, ignoreConsumed);
		}

		bool isKeyReleased(rynx::key::physical key, bool ignoreConsumed = false) const {
			return userIO->isKeyReleased(key, ignoreConsumed);
		}

		bool isKeyConsumed(rynx::key::logical key) const {
			return userIO->isKeyConsumed(keyBindings.find(key)->second);
		}

		bool isKeyConsumed(rynx::key::physical key) const {
			return userIO->isKeyConsumed(key);
		}

		void consume(rynx::key::logical key) {
			return userIO->consume(keyBindings.find(key)->second);
		}

		void consume(rynx::key::physical key) {
			return userIO->consume(key);
		}

		// NOTE: getAny* functions are not guaranteed to return correct value in cases where you have bound several gameKey codes to the same physical key.
		rynx::key::logical getAnyClicked_LogicalKey() {
			auto clicked = userIO->getAnyClicked();
			if (clicked.id == 0)
				return rynx::key::invalid_logical_key;
			return reverseBindings.find(clicked)->second.front();
		}

		rynx::key::physical getAnyClicked_PhysicalKey() {
			return userIO->getAnyClicked();
		}

		rynx::key::logical getAnyReleased_LogicalKey() {
			auto released = userIO->getAnyReleased();
			if (released.id == 0)
				return rynx::key::invalid_logical_key;
			return { reverseBindings.find(released)->second.front() };
		}

		rynx::key::physical getAnyReleased_PhysicalKey() {
			return userIO->getAnyReleased();
		}

	private:
		int32_t gameKeyCounter = 0;
		std::shared_ptr<rynx::input> userIO;
		
		rynx::unordered_map<rynx::key::logical, rynx::key::physical, rynx::key::logical::hash> keyBindings;
		rynx::unordered_map<rynx::key::physical, std::vector<rynx::key::logical>, rynx::key::physical::hash> reverseBindings;
		rynx::unordered_map<std::string, rynx::key::logical> applicationKeyByName_map;

		vec3<float> m_mouseWorldPos; // TODO: is this an ok place to have this?
	};
}