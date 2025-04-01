#pragma once

#include <memory>
#include <SDL3/SDL.h>

#include "DM3DContext.h"
#include "DMAssetRegistry.h"
#include "DMEditorAssets.h"
#include "DMEditorDialog.h"
#include "DMEditorTerrain.h"
#include "DMEngine.h"

namespace dm::editor
{
	class Editor
	{
	public:
		Editor(SDL_Window* pWindow, uint32_t width, uint32_t height);
		~Editor();

		void Run();
		core::FileSystem* GetFileSystem() const { return _engine->GetFileSystem(); }
		core::AssetRegistry* GetAssetRegistry() const { return &_engine->GetWorld()->assetRegistry; }
		model::WorldModel* GetWorld() const { return _engine->GetWorld(); }
		dm3d::Extent2D GetScreenSize() const { return _engine->GetGpuContext()->get_current_draw_extent(); }

		
	private:
		void ProcessDialogs();
		void DrawUI();
		void DrawCellList();
		void InitNewWorld();
		void SaveWorld();

		LoggerContext _log = LoggerContext("Editor");
		std::unique_ptr<dm::Engine> _engine;

		// ui state
		std::queue<std::shared_ptr<Dialog>> _dialogs;
		dm::model::Cell* _selectedCell = nullptr;
		core::GameObject* _selectedGameObject = nullptr;
		TerrainEditor _terrainEditor;
		AssetEditor _assetEditor;
		// end ui state

		
	public:
		template<typename T>
		void OpenDialog()
		{
			auto newDialog = std::make_shared<T>(this);
			_dialogs.push(newDialog);
		}

		template<typename T, typename V>
		void OpenDialog(V data)
		{
			auto newDialog = std::make_shared<T>(this, data);
			_dialogs.push(newDialog);
		}
	};
}
