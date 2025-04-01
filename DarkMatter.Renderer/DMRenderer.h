#pragma once

#include "DM3DContext.h"
#include "DMAssetManager.h"
#include "DMConstantBufferCache.h"
#include "DMLogger.h"
#include "DMShaderCache.h"
#include "DMWorldModel.h"
#include "SharedShaderTypes.h"

class dm3d::Context;

namespace dm
{
	class Engine;
}

namespace dm::renderer
{
	class Renderer
	{
		friend class dm::Engine;
	public:
		Renderer(SDL_Window* pWindow, uint32_t width, uint32_t height);
		~Renderer();

		void SetWorld(model::WorldModel* pWorldModel, core::FileSystem* pFileSystem);
		void RenderWorld();
		void RebuildSwapchain(uint32_t width, uint32_t height);

		void Present();

	private:
		struct SplatPack
		{
			std::shared_ptr<dm3d::Image> texture1;
			std::shared_ptr<dm3d::Image> texture2;
			std::shared_ptr<dm3d::Image> texture3;
			std::shared_ptr<dm3d::Image> texture4;
		};
	private:
		void InitTerrainResources();

		void RenderTerrain(model::WorldModel* pWorld);
		void RenderSky(model::WorldModel* pWorld);
		void RebuildHeightmap(model::TerrainHeightMap* pHeightMap);
		void RebuildHeightmapOverlay(model::TerrainHeightMap* pHeightMap);
		void RebuildCellSplatMap(model::TerrainHeightMap* pHeightMap);

		std::optional<SplatPack> GetSplatPack(const model::Cell& pCell) const;

		LoggerContext _log = LoggerContext("Renderer");
		uint64_t _frameNum = 0;
		std::unique_ptr<dm3d::Context> _context;
		std::unique_ptr<ShaderCache> _shaderCache;

		std::shared_ptr<dm3d::Image> _depthBuffer;
		std::shared_ptr<dm3d::Image> _heightMap;
		std::shared_ptr<dm3d::Image> _heightMapOverlay;
		std::shared_ptr<dm3d::Image> _heightMapSplat;

		// terrain
		std::unique_ptr<ConstantBufferCache<TerrainCellDrawData>> _terrainDrawDataCache;
		core::MeshRenderable _terrainLowLod;
		core::MeshRenderable _terrainMid1Lod;
		core::MeshRenderable _terrainMid2Lod;
		core::MeshRenderable _terrainHighLod;

		// asset
		model::WorldModel* _worldModel = nullptr;
		std::unique_ptr<core::AssetManager> _assetManager = nullptr;
	private:
		void EnsureMeshLoaded(core::MeshRenderable& renderable)
		{
			if (renderable.vertexBuffer == nullptr)
			{
				_log.warning("EnsureMeshLoaded: loading mesh");
				renderable.vertexBuffer = _context->build_structured(renderable.vertices, false);

				renderable.indexBuffer = _context->create_index_buffer(renderable.indices.data(), renderable.indices.size());

				renderable.numIndices = static_cast<uint32_t>(renderable.indices.size());
				renderable.vertices = std::vector<DMVertex>();
				renderable.indices = std::vector<uint32_t>();
			}
		}
	};
}