#include <rynx/menu/Component.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/system/assert.hpp>

#include <rynx/menu/Frame.hpp> // for background
#include <rynx/menu/Div.hpp> // for menu system root element

void rynx::menu::Component::privateAlign(Align sideOf, float mul) {
	if (sideOf & LEFT) {
		m_horizontalAttachment.align.x = -0.5f;
		m_horizontalAttachment.innerOuterMultiplier = mul;
	}
	if (sideOf & RIGHT) {
		m_horizontalAttachment.align.x = +0.5f;
		m_horizontalAttachment.innerOuterMultiplier = mul;
	}
	if (sideOf & CENTER_HORIZONTAL) {
		m_horizontalAttachment.align.x = 0.0f;
		m_horizontalAttachment.innerOuterMultiplier = mul;
	}

	if (sideOf & TOP) {
		m_verticalAttachment.align.y = +0.5f;
		m_verticalAttachment.innerOuterMultiplier = mul;
	}
	if (sideOf & BOTTOM) {
		m_verticalAttachment.align.y = -0.5f;
		m_verticalAttachment.innerOuterMultiplier = mul;
	}
	if (sideOf & CENTER_VERTICAL) {
		m_verticalAttachment.align.y = 0.0f;
		m_verticalAttachment.innerOuterMultiplier = mul;
	}
}

void rynx::menu::Component::privateAttach(Component* other, Align align) {
	if (align & (LEFT | RIGHT | CENTER_HORIZONTAL)) {
		m_horizontalAttachment.attachedTo = other;
		m_horizontalAttachment.is_attached = true;
	}
	if (align & (TOP | BOTTOM | CENTER_VERTICAL)) {
		m_verticalAttachment.attachedTo = other;
		m_verticalAttachment.is_attached = true;
	}
}


void rynx::menu::Component::updateAttachment() {
	if (m_horizontalAttachment) {
		Component* attached_to = ((m_horizontalAttachment.attachedTo == nullptr) ? m_pParent : m_horizontalAttachment.attachedTo);

		float offset_x_value = 0;
		if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToSelfScale) {
			offset_x_value = scale_world().x * 2.0f * m_horizontalAttachment.offset * m_horizontalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToHostScale) {
			offset_x_value = attached_to->scale_world().x * 2.0f * m_horizontalAttachment.offset * m_horizontalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToParentScale) {
			offset_x_value = parent()->scale_world().x * 2.0f * m_horizontalAttachment.offset * m_horizontalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::Absolute) {
			offset_x_value = m_horizontalAttachment.offset * m_horizontalAttachment.innerOuterMultiplier;
		}

		if (m_horizontalAttachment.align.x != 0.0f) {
			m_position.target_value().x = attached_to->position_relative(m_horizontalAttachment.align).x;
			m_position.target_value().x += (scale_world().x + offset_x_value) * m_horizontalAttachment.align.x * m_horizontalAttachment.innerOuterMultiplier;
		}
		else {
			m_position.target_value().x = attached_to->position_world().x;
		}
	}
	if (m_verticalAttachment) {
		Component* attached_to = m_verticalAttachment.attachedTo == nullptr ? m_pParent : m_verticalAttachment.attachedTo;

		float offset_y_value = 0;
		if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToSelfScale) {
			offset_y_value = scale_world().y * 2.0f * m_verticalAttachment.offset * m_verticalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToHostScale) {
			offset_y_value = attached_to->scale_world().y * 2.0f * m_verticalAttachment.offset * m_verticalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::RelativeToParentScale) {
			offset_y_value = parent()->scale_world().y * 2.0f * m_verticalAttachment.offset * m_verticalAttachment.innerOuterMultiplier;
		}
		else if (m_horizontalAttachment.offsetKind == offset_kind::Absolute) {
			offset_y_value = m_verticalAttachment.offset * m_verticalAttachment.innerOuterMultiplier;
		}

		if (m_verticalAttachment.align.y != 0.0f) {
			m_position.target_value().y = attached_to->position_relative(m_verticalAttachment.align).y;
			m_position.target_value().y += (scale_world().y + offset_y_value) * m_verticalAttachment.align.y * m_verticalAttachment.innerOuterMultiplier;
		}
		else {
			m_position.target_value().y = attached_to->position_world().y;
		}
	}
}

void rynx::menu::Component::updatePosition() {
	if (!m_pParent || m_horizontalAttachment || m_verticalAttachment) {
		m_worldPosition = m_position; // attached objects follow their host.
	}
	else {
		// other components follow their parent.
		m_worldPosition = m_position + m_pParent->position_world();
		// m_worldPosition = m_position + m_pParent->position_relative({0, +1, 0});
	}
}

void rynx::menu::Component::updateScale() {
	if (m_dynamic_scale) {
		float min_x = +1.0f;
		float max_x = -1.0f;
		float min_y = +1.0f;
		float max_y = -1.0f;
		for (auto&& child : m_children) {
			auto pos = child->position_world();
			auto scale = child->scale_world();
			min_x = std::min(min_x, pos.x - scale.x);
			max_x = std::max(max_x, pos.x + scale.x);
			min_y = std::min(min_y, pos.y - scale.y);
			max_y = std::max(max_y, pos.y + scale.y);
		}

		auto desired_position = rynx::vec3f((max_x+min_x) * 0.5f, (max_y+min_y) * 0.5f, 0);
		auto desired_scale = rynx::vec3f(max_x-min_x, max_y-min_y, 0.0f);
		m_worldScale = desired_scale * 0.5f;
		m_worldScale.y *= 2.0f;
	}
	else {
		if (m_pParent == nullptr || m_ignore_parent_scale) {
			m_worldScale = m_scale;
		}
		else {
			m_worldScale = m_scale * m_pParent->scale_world();
		}

		if (m_respect_aspect_ratio) {
			m_worldScale.y *= m_aspectRatio;
		}
	}
}

rynx::menu::Component::Component(
	vec3<float> scale,
	vec3<float> position
) {
	m_pParent = nullptr;
	m_position = position;
	m_scale = scale;
	m_color = {1.0f, 1.0f, 1.0f, 1.0f};
	m_color.tick(1.0f);
}

void rynx::menu::Component::set_parent(Component* other) {
	m_pParent = other;
}

void rynx::menu::Component::set_background(rynx::graphics::GPUTextures& textures, std::string texture, float edge_size) {
	m_background = std::make_unique<rynx::menu::Frame>(textures, texture, edge_size);
	m_background->set_parent(this);
}

void rynx::menu::Component::addChild(std::shared_ptr<Component> child) {
	child->m_pParent = this;
	child->m_menuSystem = this->m_menuSystem;
	
	std::function<void(Component*)> recursiveSystemUpdate = [menuSys = this->m_menuSystem, &recursiveSystemUpdate](Component* component) {
		for (auto&& child : component->m_children) {
			child->m_menuSystem = menuSys;
			recursiveSystemUpdate(child.get());
		}
	};
	recursiveSystemUpdate(child.get());

	if (m_dynamic_scale) {
		child->m_ignore_parent_scale = true;
	}
	m_children.emplace_back(std::move(child));
}

std::shared_ptr<rynx::menu::Component> rynx::menu::Component::detachChild(const Component* ptr) {
	auto it = std::find_if(m_children.begin(), m_children.end(), [ptr](const std::shared_ptr<Component>& child) { return child.get() == ptr; });
	rynx_assert(it != m_children.end(), "detached child must exist");
	std::shared_ptr<Component> detachedChild = std::move(*it);
	m_children.erase(it);
	return detachedChild;
}

void rynx::menu::Component::reparent(Component& other) {
	rynx_assert(m_pParent, "must have parent");
	other.addChild(m_pParent->detachChild(this));
}


void rynx::menu::Component::input(rynx::mapped_input& input) {
	if (m_active) {
		// input is handled in reverse order. the last thing drawn on the screen is the first to
		// handle input. in case multiple buttons are drawn on top of each other (for example some menu layer + popup dialog)
		// the top most (last drawn) menu element should be the one to consume any mouse clicks.
		for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
			(*it)->input(input);
		}

		rynx::vec3f mousePos = input.mouseMenuPosition(m_aspectRatio);
		if (!m_on_hover.empty()) {
			bool inRect = inRectComponent(mousePos);
			bool accepted = false;
			for (auto&& hover_func : m_on_hover)
				accepted |= hover_func(mousePos, inRect);

			if (accepted) {
				if (!m_on_click.empty()) {
					auto mousePrimary = input.getMouseKeyPhysical(0);
					bool wasClicked = input.isKeyClicked(input.getMouseKeyPhysical(0));
					input.consume(mousePrimary);
					if (wasClicked) {
						for(auto&& click_func : m_on_click)
							click_func();
					}
				}
			}
		}
		else {
			if (inRectComponent(mousePos)) {
				if (!m_on_click.empty()) {
					auto mousePrimary = input.getMouseKeyPhysical(0);
					bool wasClicked = input.isKeyClicked(input.getMouseKeyPhysical(0));
					input.consume(mousePrimary);
					if (wasClicked) {
						for(auto&& click_func : m_on_click)
							click_func();
					}
				}
			}
		}

		for(auto&& input_func : m_on_input)
			input_func(input);

		onInput(input);
	}
}

void rynx::menu::Component::tick(float dt, float aspectRatio) {
	m_aspectRatio = aspectRatio;

	m_position.tick(std::min(0.666f, dt * 5 * m_position_update_velocity));
	m_scale.tick(std::min(0.666f, dt * 8 * m_scale_update_velocity));
	m_color.tick(std::min(1.0f, dt * 5));

	if (m_background) {
		m_background->tick(dt, aspectRatio);
		m_background->color(m_color);
	}

	updateAttachment();
	updatePosition();
	updateScale();

	update(dt);
	for (auto child : m_children) {
		child->tick(dt, aspectRatio);
	}
}

void rynx::menu::Component::visualise(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const {
	if (inScreen()) {
		if (m_background) {
			m_background->draw(meshRenderer, textRenderer);
		}
		draw(meshRenderer, textRenderer);
	}
	for (auto child : m_children) {
		child->visualise(meshRenderer, textRenderer);
	}
}

bool rynx::menu::Component::inRectComponent(float x, float y) const {
	float halfWidth = m_worldScale.x * 0.5f;
	float halfHeight = m_worldScale.y * 0.5f;
	bool x_inComponent = !(x < m_worldPosition.x - halfWidth || x > m_worldPosition.x + halfWidth);
	bool y_inComponent = !(y < m_worldPosition.y - halfHeight || y > m_worldPosition.y + halfHeight);
	return x_inComponent && y_inComponent;
}

bool rynx::menu::Component::inScreen() const {
	bool out = false;
	float inverseAR = 1.0f / m_aspectRatio;
	out |= m_worldPosition.x + m_worldScale.x * 0.5f < -1.0f;
	out |= m_worldPosition.x - m_worldScale.x * 0.5f > +1.0f;
	out |= m_worldPosition.y + m_worldScale.y * 0.5f < -1.0f * inverseAR;
	out |= m_worldPosition.y - m_worldScale.y * 0.5f > +1.0f * inverseAR;
	return !out;
}

rynx::vec3<float> rynx::menu::Component::position_local() const {
	return m_position;
}

rynx::vec3<float> rynx::menu::Component::scale_local() const {
	return m_scale;
}

const rynx::vec3<float>& rynx::menu::Component::position_world() const {
	return m_worldPosition;
}

const rynx::vec3<float>& rynx::menu::Component::scale_world() const {
	return m_worldScale;
}

rynx::menu::Component& rynx::menu::Component::position_local(vec3<float> pos) {
	m_position = pos;
	return *this;
}

rynx::menu::Component& rynx::menu::Component::scale_local(vec3<float> scale) {
	m_scale = scale;
	return *this;
}

rynx::vec3<float> rynx::menu::Component::position_exterior_absolute(Align positionAlign_) const {
	vec3<float> result = m_worldPosition;
	if (positionAlign_ & LEFT)
		result.x -= m_worldScale.x * 0.5f;
	if (positionAlign_ & RIGHT)
		result.x += m_worldScale.x * 0.5f;
	if (positionAlign_ & TOP)
		result.y += m_worldScale.y * 0.5f;
	if (positionAlign_ & BOTTOM)
		result.y -= m_worldScale.y * 0.5f;
	return result;
}

rynx::vec3<float> rynx::menu::Component::position_relative(vec3<float> direction) const {
	return m_worldPosition + m_worldScale * direction;
}

rynx::vec3<float>& rynx::menu::Component::target_scale() {
	return m_scale.target_value();
}

rynx::vec3<float>& rynx::menu::Component::target_position() {
	return m_position.target_value();
}













// system

rynx::menu::System::System() {
	m_root = std::make_unique<rynx::menu::Div>(rynx::vec3f(2.0f, 2.0f, 0.0f));
	m_root->m_menuSystem = this;
}

void rynx::menu::System::input(rynx::mapped_input& input) {
	if (m_keyboardInputCapturedBy || m_mouseInputCapturedBy) {
		if (m_keyboardInputCapturedBy == m_mouseInputCapturedBy) {
			m_keyboardInputCapturedBy->onDedicatedInput(input);
		}
		else {
			if (m_keyboardInputCapturedBy) {
				auto inhibit = input.inhibit_mouse_scoped();
				m_keyboardInputCapturedBy->onDedicatedInput(input);
			}
			if (m_mouseInputCapturedBy) {
				auto inhibit = input.inhibit_keyboard_scoped();
				m_mouseInputCapturedBy->onDedicatedInput(input);
			}
		}
	}

	if (m_keyboardInputCapturedBy && m_mouseInputCapturedBy) {
		auto inhibit = input.inhibit_mouse_and_keyboard_scoped();
		m_root->input(input);
	}
	else if (m_keyboardInputCapturedBy) {
		auto inhibit = input.inhibit_keyboard_scoped();
		m_root->input(input);
	}
	else if (m_mouseInputCapturedBy) {
		auto inhibit = input.inhibit_mouse_scoped();
		m_root->input(input);
	}
	else {
		m_root->input(input);
	}
}



void rynx::menu::System::capture_mouse_input(Component* component) {
	release_mouse_input();
	m_mouseInputCapturedBy = component;
	component->onDedicatedInputGained();
}

void rynx::menu::System::capture_keyboard_input(Component* component) {
	release_keyboard_input();
	m_keyboardInputCapturedBy = component;
	component->onDedicatedInputGained();
}

void rynx::menu::System::release_mouse_input() {
	if (m_mouseInputCapturedBy) {
		m_mouseInputCapturedBy->onDedicatedInputLost();
	}
	m_mouseInputCapturedBy = nullptr;
}

void rynx::menu::System::release_keyboard_input() {
	if (m_keyboardInputCapturedBy) {
		m_keyboardInputCapturedBy->onDedicatedInputLost();
	}
	m_keyboardInputCapturedBy = nullptr;
}

rynx::scoped_input_inhibitor rynx::menu::System::inhibit_dedicated_inputs(rynx::mapped_input& input) {
	if (m_mouseInputCapturedBy && m_keyboardInputCapturedBy) {
		return input.inhibit_mouse_and_keyboard_scoped();
	}
	else if (m_mouseInputCapturedBy) {
		return input.inhibit_mouse_scoped();
	}
	else if (m_keyboardInputCapturedBy) {
		return input.inhibit_keyboard_scoped();
	}
	return {};
}

void rynx::menu::System::update(float dt, float aspectRatio) {
	m_root->tick(dt, aspectRatio);
}

void rynx::menu::System::draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) {
	m_root->visualise(meshRenderer, textRenderer);
}
