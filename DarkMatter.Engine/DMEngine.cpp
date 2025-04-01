#include "pch.h"
#include "DMEngine.h"

#include "DMCamera.h"
#include "DMInputSystem.h"
#include <format>
#include "imgui.h"

namespace dm
{
	Engine::Engine(SDL_Window* pWindow, uint32_t width, uint32_t height)
	{
		_pWindow = pWindow;
		_renderer = std::make_unique<renderer::Renderer>(_pWindow, width, height);
		core::InputSystem::Init();
		core::task::GTaskSystem = new core::task::TaskSystem;
	}

	Engine::~Engine()  // NOLINT(modernize-use-equals-default) These have to be released in a certain order
	{
		_log.information("Shutting down...");
		_world.reset();
		_renderer.reset();
		delete core::task::GTaskSystem;
		_log.information("Shutdown complete");
	}

	model::WorldModel* Engine::GetWorld()
	{
		return _world.get();
	}

	void Engine::MountFileSystem(std::unique_ptr<core::FileSystem> fileSystem)
	{
		_log.information("Mounting new file system");
		_fileSystem = std::move(fileSystem);
	}

	std::unique_ptr<core::FileSystem> Engine::UnMountFileSystem()
	{
		_log.information("Unmounting file system");

		if (_fileSystem == nullptr)
		{
			_log.error("Failed to unmount filesystem, _fileSystem was nullptr");
			return nullptr;
		}

		auto fs = std::move(_fileSystem);
		_fileSystem = nullptr;
		return fs;
	}

	void Engine::Render()
	{
		if (_world != nullptr)
			_renderer->RenderWorld();

		_renderer->Present();
	}

	void Engine::Tick()
	{
		if (_world != nullptr)
		{
			for (auto& obj : _world->globalObjectStore)
			{
				obj->Tick();
			}

			auto cam = std::dynamic_pointer_cast<core::Camera>(_world->globalObjectStore[_world->activeCamera]);
			ImGui::Begin("Camera stats");
			ImGui::Text(std::format("X: {}, Y: {}, Z: {}", cam->position.x, cam->position.y, cam->position.z).c_str());
			ImGui::End();
		}

		core::InputSystem::inputState.mouseDelta = {};
	}

	void Engine::Shutdown()
	{
		
	}

	void Engine::OnResize(int newWidth, int newHeight)
	{
		_renderer->RebuildSwapchain(newWidth, newHeight);

		if (_world != nullptr)
		{
			auto camera = std::dynamic_pointer_cast<core::Camera>(_world->globalObjectStore[_world->activeCamera]);
			camera->aspectRatio = static_cast<float>(newWidth) / static_cast<float>(newHeight);
		}
	}

	std::unique_ptr<model::WorldModel> Engine::SetWorldModel(std::unique_ptr<model::WorldModel> newModel)
	{
		auto oldModel = std::move(_world);
		_world = std::move(newModel);

		_renderer->SetWorld(_world.get(), _fileSystem.get());

		return oldModel;
	}



}
