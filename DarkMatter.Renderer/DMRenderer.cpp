#include "pch.h"
#include "DMRenderer.h"

#include "DMGlobalSettings.h"
#include "DMUtilities.h"

namespace dm::renderer
{
	Renderer::Renderer(SDL_Window* pWindow, uint32_t width, uint32_t height)
	{
		_log.information("Initializing renderer...");
		_context = std::make_unique<dm3d::Context>(dm3d::Extent2D{ .width = width, .height = height }, pWindow, core::GSettings.DebugDirectX);

		_depthBuffer = _context->create_image(dm3d::Extent3D{ .width = width, .height = height, .depth = 1 }, dm3d::D32_FLOAT, dm3d::DepthStencil, dm3d::ResourceState::ShaderRead, "DepthBuffer");
		_context->register_depth_stencil_view(_depthBuffer);

		_shaderCache = std::make_unique<ShaderCache>(_context.get());

		InitTerrainResources();

		_log.information("Renderer initialized");
	}

	Renderer::~Renderer()
	{
		_log.information("Shutting down...");
		_context->wait_for_idle();

		// these must be released in a specific order
		_terrainDrawDataCache.reset();
		_terrainLowLod = {};
		_terrainMid2Lod = {};
		_terrainMid1Lod = {};
		_terrainHighLod = {};
		_shaderCache.reset();
		_depthBuffer.reset();
		_heightMap.reset();
		_heightMapOverlay.reset();
		_heightMapSplat.reset();
		_assetManager.reset();
		_context.reset();
		_log.information("Shutdown complete");
	}

	void Renderer::SetWorld(model::WorldModel* pWorldModel, core::FileSystem* pFileSystem)
	{
		assert(_worldModel == nullptr && _assetManager == nullptr);
		_worldModel = pWorldModel;

		_assetManager = std::make_unique<core::AssetManager>();
		_assetManager->SetFileSystem(pFileSystem);
		_assetManager->SetGpuContext(_context.get());
		_assetManager->SetRegistry(&pWorldModel->assetRegistry);
	}

	void Renderer::RebuildSwapchain(uint32_t width, uint32_t height)
	{
		_context->rebuild_swapchain(dm3d::Extent2D{ .width = width, .height = height });

		_depthBuffer = _context->create_image(dm3d::Extent3D{ .width = width, .height = height, .depth = 1 }, dm3d::D32_FLOAT, dm3d::DepthStencil, dm3d::ResourceState::ShaderRead, "DepthBuffer");
		_context->register_depth_stencil_view(_depthBuffer);
	}

	void Renderer::RenderWorld()
	{
		assert(_worldModel != nullptr);

		RenderSky(_worldModel);
		RenderTerrain(_worldModel);
	}

	void Renderer::Present()
	{
		_context->present();

		_terrainDrawDataCache->Reset();

		auto cmd = _context->allocate_command_list();
		auto backBuffer = _context->get_back_buffer();
		cmd->try_defer_transition(backBuffer, dm3d::ResourceState::RenderTarget);
		cmd->try_defer_transition(_depthBuffer, dm3d::ResourceState::DepthWrite);
		float clearColor[] = { 0.f, 0.f, 0.f, 0.f };
		cmd->clear_image(clearColor, backBuffer.get());
		cmd->clear_depth(1.f, _depthBuffer);
		_context->submit_list(std::move(cmd));
	}

}
