#include "pch.h"
#include "EditorCamera.h"

#include <algorithm>

#include "DMInputSystem.h"

EditorCamera::EditorCamera() : Camera(), _speed(10.f), _freeLook(false)
{

}

EditorCamera::EditorCamera(const EditorCamera& other) : Camera(other), _speed(other._speed), _freeLook(other._freeLook)
{

}

std::shared_ptr<dm::core::GameObject> EditorCamera::DeepClone()
{
	auto cam = std::make_shared<EditorCamera>(*this);
	return cam;
}

void EditorCamera::Tick()
{
	auto moveDir = glm::vec3(0.f);

	if (dm::core::InputSystem::Is(dm::core::InputButton::W, dm::core::ButtonState::Down))
	{
		moveDir.z = -1.f;
	}
	if (dm::core::InputSystem::Is(dm::core::InputButton::S, dm::core::ButtonState::Down))
	{
		moveDir.z = 1.f;
	}
	if (dm::core::InputSystem::Is(dm::core::InputButton::A, dm::core::ButtonState::Down))
	{
		moveDir.x = -1.f;
	}
	if (dm::core::InputSystem::Is(dm::core::InputButton::D, dm::core::ButtonState::Down))
	{
		moveDir.x = 1.f;
	}

	_freeLook = dm::core::InputSystem::Is(dm::core::InputButton::RightMouse, dm::core::ButtonState::Down);

	HandleMouse(dm::core::InputSystem::inputState.mouseDelta);
	HandleMove(moveDir);
}

void EditorCamera::HandleMove(glm::vec3 moveVector)
{
	glm::mat4 rotation = GetRotationMatrix();
	auto velocity = moveVector * _speed;
	position += glm::vec3(rotation * glm::vec4(velocity * 0.5f, 0.f));
}

void EditorCamera::HandleMouse(glm::vec2 delta)
{
	if (!_freeLook)
		return;
	yaw += delta.x / 200.f;
	pitch -= delta.y / 200.f;

	pitch = std::clamp(pitch, -1.5f, 1.5f);
}