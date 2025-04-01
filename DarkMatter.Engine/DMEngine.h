#pragma once
#include <memory>
#include <SDL3/SDL_events.h>

#include "DMRenderer.h"
#include <SDL3/SDL_video.h>

#include "DMFileSystem.h"

namespace dm
{
	namespace editor
	{
		class Editor;
	}

	class Engine
	{
		friend class dm::editor::Editor;
	public:
		Engine(SDL_Window* pWindow, uint32_t width, uint32_t height);
		~Engine();

		model::WorldModel* GetWorld();
		void MountFileSystem(std::unique_ptr<core::FileSystem> fileSystem);
		std::unique_ptr<core::FileSystem> UnMountFileSystem();
		void HandleEvent(SDL_Event* event);
		void Tick();
		void Render();
		void Shutdown();
		void OnResize(int newWidth, int newHeight);
		dm3d::Context* GetGpuContext() const { return _renderer->_context.get(); }
		core::FileSystem* GetFileSystem() const { return _fileSystem.get(); }
		std::unique_ptr<model::WorldModel> SetWorldModel(std::unique_ptr<model::WorldModel> newModel);
		void LoadFromFolder(const std::string& path);
		void SaveToFolder(const std::string& path) const;
		void LoadAssetsRegistryFile(const std::string& path) const;
	private:
		

		LoggerContext _log = LoggerContext("Engine");

		SDL_Window* _pWindow;

		std::unique_ptr<dm::model::WorldModel> _world;
		std::unique_ptr<dm::renderer::Renderer> _renderer;
		std::unique_ptr<dm::core::FileSystem> _fileSystem;
	};
}
