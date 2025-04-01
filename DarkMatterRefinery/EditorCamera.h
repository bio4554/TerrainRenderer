#pragma once
#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "DMCamera.h"
#include "DMGameObject.h"

class EditorCamera : public dm::core::Camera
{
public:
	EditorCamera();
	EditorCamera(const EditorCamera& other);

	std::shared_ptr<dm::core::GameObject> DeepClone() override;

	void Tick() override;

private:
	void HandleMove(glm::vec3 moveVector);
	void HandleMouse(glm::vec2 delta);

	float _speed;
	bool _freeLook;
};
