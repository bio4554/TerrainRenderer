#include "pch.h"
#include "DMEditorCamera.h"

#include <algorithm>

#include "DMInputSystem.h"
#include "imgui.h"

namespace dm::editor
{
	EditorCamera::EditorCamera() : Camera(), _speed(10.f), _freeLook(false)
	{
		
	}

	EditorCamera::EditorCamera(const EditorCamera& other) : Camera(other), _speed(other._speed), _freeLook(other._freeLook)
	{
		
	}

	std::shared_ptr<core::GameObject> EditorCamera::DeepClone()
	{
		auto cam = std::make_shared<EditorCamera>(*this);
		return cam;
	}

	void EditorCamera::Tick()
	{
		ImGui::Begin("Camera settings");
		ImGui::InputFloat("Speed", &_speed);
		ImGui::End();

		auto moveDir = glm::vec3(0.f);

		if (core::InputSystem::Is(core::InputButton::W, core::ButtonState::Down))
		{
			moveDir.z = -1.f;
		}
		if (core::InputSystem::Is(core::InputButton::S, core::ButtonState::Down))
		{
			moveDir.z = 1.f;
		}
		if (core::InputSystem::Is(core::InputButton::A, core::ButtonState::Down))
		{
			moveDir.x = -1.f;
		}
		if (core::InputSystem::Is(core::InputButton::D, core::ButtonState::Down))
		{
			moveDir.x = 1.f;
		}

		_freeLook = core::InputSystem::Is(core::InputButton::RightMouse, core::ButtonState::Down);

		HandleMouse(core::InputSystem::inputState.mouseDelta);
		HandleMove(moveDir);
	}

	void EditorCamera::HandleMove(glm::vec3 moveVector)
	{
		auto speed = _speed;

		if (core::InputSystem::Is(core::InputButton::Shift, core::ButtonState::Down))
		{
			speed = speed * 10.f;
		}

		glm::mat4 rotation = GetRotationMatrix();
		auto velocity = moveVector * speed;
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
}
