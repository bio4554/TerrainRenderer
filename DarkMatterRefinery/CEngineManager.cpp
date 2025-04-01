#include "pch.h"
#include "CEngineManager.h"

#include <ranges>

#include "DMEAppMessages.h"
#include "DMEViewRegistry.h"
#include "DMInputSystem.h"
#include "DMRealFileSystem.h"
#include "DMTextureTools.h"
#include "DMUtilities.h"
#include "EditorCamera.h"
#include "imgui_impl_sdl3.h"

CEngineManager* GlobalEngineManager;
dm::Engine* GlobalEngine;

void CEngineManager::Init(HWND hWnd)
{
	if (SDL_Init(SDL_INIT_VIDEO) == false)
	{
		auto err = SDL_GetError();
		std::cerr << "SDL_Init error: " << err << '\n';
		return;
	}

	auto winProps = SDL_CreateProperties();
	SDL_SetPointerProperty(winProps, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, hWnd);
	SDL_SetBooleanProperty(winProps, SDL_PROP_WINDOW_CREATE_FOCUSABLE_BOOLEAN, true);
	SDL_SetBooleanProperty(winProps, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);

	auto sdlWindow = SDL_CreateWindowWithProperties(winProps);

	if (sdlWindow == nullptr)
	{
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
		SDL_Quit();
		return;
	}

	_engine = std::make_unique<dm::Engine>(sdlWindow, 1920, 1080);

	GlobalEngine = _engine.get();

	auto fileSystem = std::make_unique<dm::core::RealFileSystem>();
	fileSystem->Mount("C:\\Users\\bio4554\\source\\repos\\TerrainRenderer\\project_assets");
	_engine->MountFileSystem(std::move(fileSystem));

	InitNewWorld();

	_engine->LoadFromFolder("meta");

	_engineThread = std::thread([&]()
		{
			HRESULT hr = SetThreadDescription(GetCurrentThread(), L"Engine Thread");
			SDL_Event e;
			while (!_doQuit)
			{
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
							_doQuit = true;
						}
					}

					ImGui::ShowDemoWindow();

					_engine->Tick();
					_engine->Render();

					if (_doQuit)
					{
						_engine->Shutdown();
					}

					_engineSync.AcknowledgeLock();
				}
			}
		});

	GViews.GetWorldView()->PostMessageW(WM_DME_CELL_CHANGED);
}

void CEngineManager::Shutdown()
{
	_doQuit = true;
	_engineThread.join();

	_engine->SaveToFolder("meta");
}

void CEngineManager::SubmitTransaction(dme::EditTransaction transaction)
{
	_transactionStack.push(transaction);
	LockEngine();
	transaction.commit();
	UnlockEngine();
}

void CEngineManager::LockEngine()
{
	_engineSync.Lock();
}

void CEngineManager::UnlockEngine()
{
	_engineSync.Unlock();
}

void CEngineManager::Resize(int newWidth, int newHeight)
{
	if (newWidth > 0 && newHeight > 0 && _engine != nullptr)
	{
		_engineSync.Lock();
		_engine->OnResize(newWidth, newHeight);
		_engineSync.Unlock();
	}
}

std::vector<dme::TextureAssetMeta> CEngineManager::GetTextureAssets() const
{
	auto& textures = _engine->GetWorld()->assetRegistry.GetTextures();
	std::vector<dme::TextureAssetMeta> response;
	response.reserve(textures.size());

	for (const auto& val : textures)
	{
		dme::TextureAssetMeta meta;
		meta.id = val.first;
		meta.name = val.second.GetPath();
		auto extent = val.second.GetExtent();
		meta.extent = std::format("{}x{}x{}", extent.width, extent.height, extent.depth);
		response.push_back(meta);
	}

	return response;
}

std::vector<dme::CellMeta> CEngineManager::GetCellData() const
{
	if (_engine == nullptr)
		return std::vector<dme::CellMeta>();

	const auto& cells = _engine->GetWorld()->cellStore;
	std::vector<dme::CellMeta> response;
	response.reserve(cells.size());

	for (size_t i = 0; i < cells.size(); i++)
	{
		const auto& cell = cells[i];
		dme::CellMeta meta;
		meta.id = static_cast<uint32_t>(i);
		meta.name = "Wilderness";
		auto center = cell.GetCenter();
		meta.position = std::format("{}, {}, {}", center.x, center.y, center.z);
		response.push_back(meta);
	}

	return response;
}

void CEngineManager::ImportTexture(std::string path, std::string name)
{
	auto rawData = dm::core::utility::ReadBinaryFile(path);

	auto textureOpt = dm::asset::TextureTools::ConvertToDDS(rawData.data(), rawData.size(), name, _engine->GetFileSystem(), true, 9);
	if (textureOpt.has_value() == false)
	{
		return;
	}

	auto texture = textureOpt.value();
	auto& registry = _engine->GetWorld()->assetRegistry;
	auto newId = registry.Allocate();
	texture.SetId(newId);
	registry.Register(texture);

	auto assetView = GViews.GetAssetView();
	if (assetView != nullptr)
	{
		assetView->PostMessageW(WM_DME_TEXTURE_ASSET_CHANGED);
	}
}

void CEngineManager::InitNewWorld()
{
	std::unique_ptr<dm::model::WorldModel> world = std::make_unique<dm::model::WorldModel>();

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

			dm::model::Cell cell(topLeft, topRight, bottomLeft, bottomRight, uvTopLeft, uvTopRight, uvBottomLeft,
				uvBottomRight, 32);

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

	world->terrainHeightMap = dm::model::TerrainHeightMap(1024, 5120);

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
}
