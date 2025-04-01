#pragma once
#include <unordered_map>
#include <glm/vec2.hpp>

namespace dm::core
{
	enum class InputButton : uint32_t
	{
		W = 0,
		A,
		S,
		D,
		Escape,
		Space,
		R,
		RightMouse,
		LeftMouse,
		Quit,
		Shift,
		Count
	};

	enum class ButtonState : uint8_t
	{
		Down,
		Up
	};

	struct InputState
	{
		std::unordered_map<InputButton, ButtonState> buttons;
		glm::vec2 mouseDelta;
		glm::vec2 mousePosGlobal;
		glm::vec2 mousePosLocal;

		bool ignoreKeyboard;
		bool ignoreMouse;
	};

	class InputSystem
	{
	public:
		static void Init()
		{
			inputState.buttons[InputButton::W] = ButtonState::Up;
			inputState.buttons[InputButton::A] = ButtonState::Up;
			inputState.buttons[InputButton::S] = ButtonState::Up;
			inputState.buttons[InputButton::D] = ButtonState::Up;
			inputState.buttons[InputButton::RightMouse] = ButtonState::Up;
			inputState.buttons[InputButton::LeftMouse] = ButtonState::Up;
			inputState.buttons[InputButton::Escape] = ButtonState::Up;
			inputState.buttons[InputButton::Space] = ButtonState::Up;
			inputState.buttons[InputButton::R] = ButtonState::Up;
			inputState.buttons[InputButton::Shift] = ButtonState::Up;
		}

		static bool Is(InputButton button, ButtonState state)
		{
			return inputState.buttons[button] == state;
		}

		static InputState inputState;
	};
}
