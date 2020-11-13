#include <rynx/menu/Component.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/system/assert.hpp>

void rynx::menu::Component::privateAlign(Align sideOf, float mul) {
	if (sideOf & LEFT) {
		m_horizontalAttachment.align.x = -0.5f;
		m_horizontalAttachment.innerOuterMultiplier = mul;
	}
	if (sideOf & RIGHT) {
		m_horizontalAttachment.align.x = +0.5f;
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
}

void rynx::menu::Component::privateAttach(Component* other, Align align) {
	if (align & (LEFT | RIGHT)) {
		m_horizontalAttachment.attachedTo = other;
		m_horizontalAttachment.is_attached = true;
	}
	if (align & (TOP | BOTTOM)) {
		m_verticalAttachment.attachedTo = other;
		m_verticalAttachment.is_attached = true;
	}
}


void rynx::menu::Component::updateAttachment() {
	if (m_horizontalAttachment) {
		Component* attached_to = m_horizontalAttachment.attachedTo == nullptr ? m_pParent : m_horizontalAttachment.attachedTo;
		m_position.target_value().x = attached_to->position_relative(m_horizontalAttachment.align).x;
		m_position.target_value().x += (scale_world().x * (1.0f + 2.0f * m_horizontalAttachment.offset * m_horizontalAttachment.innerOuterMultiplier)) * m_horizontalAttachment.align.x * m_horizontalAttachment.innerOuterMultiplier;
	}
	if (m_verticalAttachment) {
		Component* attached_to = m_verticalAttachment.attachedTo == nullptr ? m_pParent : m_verticalAttachment.attachedTo;
		m_position.target_value().y = attached_to->position_relative(m_verticalAttachment.align).y;
		m_position.target_value().y += (scale_world().y * (1.0f + 2.0f * m_verticalAttachment.offset * m_verticalAttachment.innerOuterMultiplier)) * m_verticalAttachment.align.y * m_verticalAttachment.innerOuterMultiplier;
	}
}

void rynx::menu::Component::updatePosition() {
	if (m_pParent == nullptr) {
		m_worldPosition = m_position;
	}
	else {
		if (m_horizontalAttachment || m_verticalAttachment)
			m_worldPosition = m_position;
		else
			m_worldPosition = m_position + m_pParent->position_world();
	}
}

void rynx::menu::Component::updateScale() {
	if (m_pParent == nullptr) {
		m_worldScale = m_scale;
	}
	else {
		m_worldScale = m_scale * m_pParent->scale_world();
	}

	if (m_respect_aspect_ratio) {
		m_worldScale.y *= m_aspectRatio;
	}
}

rynx::menu::Component::Component(
	vec3<float> scale,
	vec3<float> position
) {
	m_pParent = nullptr;
	m_position = position;
	m_scale = scale;
}

void rynx::menu::Component::set_parent(Component* other) {
	m_pParent = other;
}

void rynx::menu::Component::addChild(std::shared_ptr<Component> child) {
	child->m_pParent = this;
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
		for (auto&& child : m_children) {
			child->input(input);
		}
	}
}

void rynx::menu::Component::tick(float dt, float aspectRatio) {
	m_aspectRatio = aspectRatio;

	m_position.tick(std::min(1.0f, dt * 5 * m_position_update_velocity));
	m_scale.tick(std::min(1.0f, dt * 8 * m_scale_update_velocity));

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