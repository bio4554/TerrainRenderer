#pragma once
#include "DMCamera.h"

namespace dm::editor
{
	class EditorCamera : public core::Camera
	{
	public:
		EditorCamera();
		EditorCamera(const EditorCamera& other);

		std::shared_ptr<GameObject> DeepClone() override;

		void Tick() override;

	private:
		void HandleMove(glm::vec3 moveVector);
		void HandleMouse(glm::vec2 delta);

		float _speed;
		bool _freeLook;
	};
}
