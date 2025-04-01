#include "pch.h"

#include "DMCamera.h"
#include "DMEngine.h"
#include "DMInputSystem.h"

namespace dm
{
	void Engine::HandleEvent(SDL_Event* event)
	{
		if (event->type == SDL_EVENT_KEY_DOWN && !core::InputSystem::inputState.ignoreKeyboard)
		{
			if (event->key.key == SDLK_W) { core::InputSystem::inputState.buttons[core::InputButton::W] = core::ButtonState::Down; }
			if (event->key.key == SDLK_S) { core::InputSystem::inputState.buttons[core::InputButton::S] = core::ButtonState::Down; }
			if (event->key.key == SDLK_A) { core::InputSystem::inputState.buttons[core::InputButton::A] = core::ButtonState::Down; }
			if (event->key.key == SDLK_D) { core::InputSystem::inputState.buttons[core::InputButton::D] = core::ButtonState::Down; }
			if (event->key.key == SDLK_R) { core::InputSystem::inputState.buttons[core::InputButton::R] = core::ButtonState::Down; }
			if (event->key.key == SDLK_LSHIFT)
			{
				core::InputSystem::inputState.buttons[core::InputButton::Shift] = core::ButtonState::Down;
			}

			if (event->key.key == SDLK_ESCAPE)
			{
				core::InputSystem::inputState.buttons[core::InputButton::Escape] = core::ButtonState::Down;
			}

			if (event->key.key == SDLK_SPACE)
			{
				core::InputSystem::inputState.buttons[core::InputButton::Space] = core::ButtonState::Down;
			}
		}

		if (event->type == SDL_EVENT_KEY_UP && !core::InputSystem::inputState.ignoreKeyboard)
		{
			if (event->key.key == SDLK_W) { core::InputSystem::inputState.buttons[core::InputButton::W] = core::ButtonState::Up; }
			if (event->key.key == SDLK_S) { core::InputSystem::inputState.buttons[core::InputButton::S] = core::ButtonState::Up; }
			if (event->key.key == SDLK_A) { core::InputSystem::inputState.buttons[core::InputButton::A] = core::ButtonState::Up; }
			if (event->key.key == SDLK_D) { core::InputSystem::inputState.buttons[core::InputButton::D] = core::ButtonState::Up; }
			if (event->key.key == SDLK_R) { core::InputSystem::inputState.buttons[core::InputButton::R] = core::ButtonState::Up; }
			if (event->key.key == SDLK_LSHIFT) { core::InputSystem::inputState.buttons[core::InputButton::Shift] = core::ButtonState::Up; }

			if (event->key.key == SDLK_ESCAPE)
			{
				core::InputSystem::inputState.buttons[core::InputButton::Escape] = core::ButtonState::Up;
			}

			if (event->key.key == SDLK_SPACE)
			{
				core::InputSystem::inputState.buttons[core::InputButton::Space] = core::ButtonState::Up;
			}
		}

		if (event->type == SDL_EVENT_MOUSE_MOTION && !core::InputSystem::inputState.ignoreMouse)
		{
			core::InputSystem::inputState.mouseDelta += glm::vec2{ (float)event->motion.xrel, (float)event->motion.yrel };
		}

		if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && !core::InputSystem::inputState.ignoreMouse)
		{
			if (event->button.button == SDL_BUTTON_RIGHT)
			{
				core::InputSystem::inputState.buttons[core::InputButton::RightMouse] = core::ButtonState::Down;
			}
			else if (event->button.button == SDL_BUTTON_LEFT)
			{
				core::InputSystem::inputState.buttons[core::InputButton::LeftMouse] = core::ButtonState::Down;
			}
		}

		if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && !core::InputSystem::inputState.ignoreMouse)
		{
			if (event->button.button == SDL_BUTTON_RIGHT)
			{
				core::InputSystem::inputState.buttons[core::InputButton::RightMouse] = core::ButtonState::Up;
			}
			else if (event->button.button == SDL_BUTTON_LEFT)
			{
				core::InputSystem::inputState.buttons[core::InputButton::LeftMouse] = core::ButtonState::Up;
			}
		}

		float mouseX, mouseY;
		SDL_GetGlobalMouseState(&mouseX, &mouseY);
		core::InputSystem::inputState.mousePosGlobal = { mouseX, mouseY };

		SDL_GetMouseState(&mouseX, &mouseY);
		core::InputSystem::inputState.mousePosLocal = { mouseX, mouseY };

		// other SDL events

		if (event->type == SDL_EVENT_WINDOW_RESIZED)
		{
			int newWidth = event->window.data1;
			int newHeight = event->window.data2;

			_renderer->RebuildSwapchain(newWidth, newHeight);

			if (_world != nullptr)
			{
				auto camera = std::dynamic_pointer_cast<core::Camera>(_world->globalObjectStore[_world->activeCamera]);
				camera->aspectRatio = static_cast<float>(newWidth) / static_cast<float>(newHeight);
			}
		}
	}

}
