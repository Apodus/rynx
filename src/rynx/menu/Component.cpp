#include <rynx/menu/Component.hpp>

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
	}
	if (align & (TOP | BOTTOM)) {
		m_verticalAttachment.attachedTo = other;
	}
}

rynx::menu::Component& rynx::menu::Component::alignToOuterEdge(Component* other, Align sideOf) {
	privateAttach(other, sideOf);
	privateAlign(sideOf, +1);
	return *this;
}

rynx::menu::Component& rynx::menu::Component::alignToInnerEdge(Component* other, Align sideOf) {
	privateAttach(other, sideOf);
	privateAlign(sideOf, -1);
	return *this;
}

void rynx::menu::Component::updateAttachment() {
	if (m_horizontalAttachment) {
		m_position.target_value().x = m_horizontalAttachment.attachedTo->position_relative(m_horizontalAttachment.align).x;
		m_position.target_value().x += scale_world().x * m_horizontalAttachment.align.x * m_horizontalAttachment.innerOuterMultiplier;
	}
	if (m_verticalAttachment) {
		m_position.target_value().y = m_verticalAttachment.attachedTo->position_relative(m_verticalAttachment.align).y;
		m_position.target_value().y += scale_world().y * m_verticalAttachment.align.y * m_verticalAttachment.innerOuterMultiplier;
	}
}

void rynx::menu::Component::updatePosition() {
	if (m_pParent == nullptr) {
		m_worldPosition = m_position;
	}
	else {
		m_worldPosition = m_position /* *m_pParent->scale_world() */ + m_pParent->position_world();
	}
}

void rynx::menu::Component::updateScale() {
	if (m_pParent == nullptr) {
		m_worldScale = m_scale;
	}
	else {
		m_worldScale = m_scale * m_pParent->scale_world();
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

rynx::menu::Component::Component(
	Component* parent,
	vec3<float> scale,
	vec3<float> position
) {
	m_pParent = parent;
	m_position = position;
	m_scale = scale;
}

void rynx::menu::Component::addChild(std::shared_ptr<Component> child) {
	m_children.emplace_back(std::move(child));
}

void rynx::menu::Component::input(rynx::mapped_input& input) {
	if (m_active) {
		onInput(input);
		for (auto&& child : m_children) {
			child->input(input);
		}
	}
}

void rynx::menu::Component::tick(float dt, float aspectRatio) {
	m_aspectRatio = aspectRatio;

	m_position.tick(dt * 5);
	m_scale.tick(dt * 8);

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

rynx::vec3<float> rynx::menu::Component::position_exterior(Align positionAlign_) const {
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