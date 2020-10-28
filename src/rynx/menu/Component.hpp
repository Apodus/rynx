

#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/tech/smooth_value.hpp>

#include <memory>
#include <string>
#include <vector>

namespace rynx {
	
	class TextRenderer;
	class MeshRenderer;
	class mapped_input;
	
	namespace menu {
		enum Align {
			NONE = 0,
			LEFT = 1,
			RIGHT = 2,
			TOP = 4,
			TOP_LEFT = 5,
			TOP_RIGHT = 6,
			BOTTOM = 8,
			BOTTOM_LEFT = 9,
			BOTTOM_RIGHT = 10,
			CENTER_VERTICAL = 16,
			CENTER_HORIZONTAL = 32
		};

		class Component {
		private:
			struct Attachment {
				Component* attachedTo = nullptr;
				vec3<float> align;
				float innerOuterMultiplier = 1;
				operator bool() const { return attachedTo != nullptr; }
			};

			Attachment m_verticalAttachment;
			Attachment m_horizontalAttachment;

			void privateAlign(Align sideOf, float mul);
			void privateAttach(Component* other, Align align);

			void updateAttachment();
			void updatePosition();
			void updateScale();

		protected:
			vec3<float> m_positionAlign;
			Component* m_pParent;
			std::vector<std::shared_ptr<Component>> m_children;

			rynx::smooth<vec3<float>> m_position;
			rynx::smooth<vec3<float>> m_scale;
			
			vec3<float> m_worldPosition;
			vec3<float> m_worldScale;

			float m_aspectRatio = 1;
			bool m_active = true;

			virtual void onInput(rynx::mapped_input& input) = 0;
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const = 0;
			virtual void update(float dt) = 0;

			Component* parent() const { return m_pParent; }
			float aspectRatio() const { return m_aspectRatio; }

		public:
			Component(
				vec3<float> scale,
				vec3<float> position = vec3<float>(0, 0, 0)
			);

			virtual ~Component() {}

			void input(rynx::mapped_input& input);
			void tick(float dt, float aspectRatio);
			void position_approach(float percentage) { m_position.tick(percentage); }
			void visualise(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const;

			Component& alignToOuterEdge(Component* other, Align sideOf);
			Component& alignToInnerEdge(Component* other, Align sideOf);

			void set_parent(Component* other);
			void addChild(std::shared_ptr<Component> child);
			std::shared_ptr<Component> detachChild(const Component* ptr);
			void reparent(Component& other);

			bool inRectComponent(float x, float y) const;
			bool inRectComponent(vec3<float> v) { return inRectComponent(v.x, v.y); }
			bool inScreen() const;

			vec3<float> position_local() const;
			const vec3<float>& position_world() const;
			
			vec3<float> scale_local() const;
			const vec3<float>& scale_world() const;
			
			Component& position_local(vec3<float> pos);
			Component& scale_local(vec3<float> scale);
			vec3<float> position_exterior(Align positionAlign_) const;
			vec3<float> position_relative(vec3<float> direction) const;
			vec3<float>& target_scale();
			vec3<float>& target_position();
		};
	}
}

inline rynx::menu::Align operator | (rynx::menu::Align a, rynx::menu::Align b) {
	return static_cast<rynx::menu::Align>(static_cast<int>(a) | static_cast<int>(b));
}