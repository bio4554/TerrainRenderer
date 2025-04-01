#include "DMEditor.h"

#include "DMEditorCamera.h"
#include "DMInputSystem.h"
#include "DMRealFileSystem.h"
#include "imgui_impl_sdl3.h"
#include "imgui_internal.h"

#include <format>

#include "DMEditorDialogImportTexture.h"
#include "DMTextureTools.h"

namespace dm::editor
{
	Editor::Editor(SDL_Window* pWindow, uint32_t width, uint32_t height) : _terrainEditor(this), _assetEditor(this)
	{
		_engine = std::make_unique<Engine>(pWindow, width, height);
		auto fileSystem = std::make_unique<core::RealFileSystem>();
		fileSystem->Mount("C:\\Users\\bio4554\\source\\repos\\TerrainRenderer\\project_assets");
		_engine->MountFileSystem(std::move(fileSystem));

		InitNewWorld();

		_engine->LoadFromFolder("meta");
	}

	Editor::~Editor()
	{
		_log.information("Shutting down...");
		_engine.reset();
		_log.information("Shutdown complete");
	}

	void Editor::Run()
	{
		bool quit = false;
		SDL_Event e;
		while (!quit)
		{
			while (SDL_PollEvent(&e) != 0)
			{
				ImGui_ImplSDL3_ProcessEvent(&e);

				auto& io = ImGui::GetIO();

				dm::core::InputSystem::inputState.ignoreKeyboard = io.WantCaptureKeyboard;
				dm::core::InputSystem::inputState.ignoreMouse = io.WantCaptureMouse;

				_engine->HandleEvent(&e);

				if (e.type == SDL_EVENT_QUIT)
				{
					quit = true;
				}
			}

			if (core::InputSystem::Is(core::InputButton::Escape, core::ButtonState::Down))
			{
				quit = true;
			}

			//ImGui::ShowDemoWindow();

			DrawUI();

			_terrainEditor.Tick();

			_engine->Tick();

			_engine->Render();

			if (quit)
			{
				_engine->Shutdown();
			}
		}
	}

	void Editor::DrawUI()
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New world")) {
					InitNewWorld();
				}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
				}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {
					SaveWorld();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		auto gpuStats = _engine->GetGpuContext()->get_usage_stats();

		ImGui::Begin("GPU Usage");
		ImGui::Text(std::format("Used memory (MB):      {}", (gpuStats.usedMemory / 1048576)).c_str());
		ImGui::Text(std::format("Available memory (MB): {}", (gpuStats.availableMemory / 1048576)).c_str());
		ImGui::Text(std::format("Used resources: {}", gpuStats.allocatedResources).c_str());
		ImGui::End();

		DrawCellList();

		_terrainEditor.RenderUI();
		_assetEditor.RenderUI();

		ProcessDialogs();
	}

	void Editor::ProcessDialogs()
	{
		std::queue<std::shared_ptr<Dialog>> newDialogQueue;

		while (!_dialogs.empty())
		{
			auto popped = _dialogs.front();
			_dialogs.pop();
			popped->Render();
			if (popped->IsOpen())
			{
				newDialogQueue.push(popped);
			}
		}

		_dialogs = newDialogQueue;
	}

	bool CellNameGetter(void* data, int index, const char** output)
	{
		model::Cell* cells = (model::Cell*)data;
		model::Cell& currentCell = cells[index];

		*output = currentCell.GetName().c_str(); // not very safe

		return true;
	}

	void Editor::DrawCellList()
	{
		static int listBoxSelected = 0;

		ImGui::Begin("World Cells");

		if (_engine->GetWorld() != nullptr)
		{
			ImGui::ListBox("Cells", &listBoxSelected, CellNameGetter, _engine->GetWorld()->cellStore.data(), _engine->GetWorld()->cellStore.size());
			_selectedCell = &_engine->GetWorld()->cellStore[listBoxSelected];
		}
		
		ImGui::End();
	}

	void Editor::InitNewWorld()
	{
		_log.information("Creating new world...");
		std::unique_ptr<model::WorldModel> world = std::make_unique<model::WorldModel>();

		const int gridWidth = 40;
		const int gridHeight = 40;
		const float cellSize = 128.0f;
		const float worldSize = gridWidth * cellSize;

		world->cellRegistry.reserve(gridWidth * gridHeight);

		for (int row = 0; row < gridHeight; row++)
		{
			for (int col = 0; col < gridWidth; col++)
			{
				glm::vec3 topLeft(col * cellSize, 0.0f, row * cellSize);
				glm::vec3 topRight((col + 1) * cellSize, 0.0f, row * cellSize);
				glm::vec3 bottomLeft(col * cellSize, 0.0f, (row + 1) * cellSize);
				glm::vec3 bottomRight((col + 1) * cellSize, 0.0f, (row + 1) * cellSize);

				glm::vec2 uvTopLeft((col * cellSize) / worldSize, (row * cellSize) / worldSize);
				glm::vec2 uvTopRight(((col + 1) * cellSize) / worldSize, (row * cellSize) / worldSize);
				glm::vec2 uvBottomLeft((col * cellSize) / worldSize, ((row + 1) * cellSize) / worldSize);
				glm::vec2 uvBottomRight(((col + 1) * cellSize) / worldSize, ((row + 1) * cellSize) / worldSize);

				model::Cell cell(topLeft, topRight, bottomLeft, bottomRight, uvTopLeft, uvTopRight, uvBottomLeft, uvBottomRight, 32);

				world->cellRegistry.push_back(cell);
			}
		}

		auto editorCamera = std::make_shared<EditorCamera>();
		editorCamera->fov = 90.f;
		editorCamera->aspectRatio = 16.f / 9.f;
		editorCamera->far = 6000.f;
		editorCamera->near = .1f;

		world->globalObjectStore.clear();
		world->globalObjectStore.push_back(editorCamera);
		world->activeCamera = 0;

		// TODO hack
		world->cellStore = world->cellRegistry;

		world->terrainHeightMap = model::TerrainHeightMap(1024, 5120);

		_engine->SetWorldModel(std::move(world));

		// TODO TEMP
		{
			auto heightmapSize = _engine->GetFileSystem()->FileSize("test/Hydro.r32");
			auto numFloats = heightmapSize / sizeof(float);
			auto heightmapRaw = std::vector<float>(numFloats);
			_engine->GetFileSystem()->ReadFile("test/Hydro.r32", reinterpret_cast<char*>(heightmapRaw.data()));

			auto w = _engine->GetWorld();

			auto& terrainFloats = w->terrainHeightMap.GetFloatData();

			for (int x = 0; x < 1024; x++)
			{
				for (int y = 0; y < 1024; y++)
				{
					terrainFloats[x][y] = heightmapRaw[x + (y * 1024)] * 5000.f;
				}
			}
		}

		_log.information("Created world");
	}

	void Editor::SaveWorld()
	{
		auto fileSystem = GetFileSystem();
		{
			// asset registry
			auto serializedAssetRegistry = GetAssetRegistry()->SerializeRegistry();
			fileSystem->WriteFile("meta/assets.json", serializedAssetRegistry.data(), serializedAssetRegistry.size(), false);
		}
	}

}
