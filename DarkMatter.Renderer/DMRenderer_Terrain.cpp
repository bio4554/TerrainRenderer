#include "pch.h"

#include "DMCamera.h"
#include "DMRenderer.h"
#include "DMUtilities.h"
#include "SharedShaderTypes.h"
#include "imgui.h"

namespace dm::renderer
{
    
	void Renderer::InitTerrainResources()
	{
        _terrainDrawDataCache = std::make_unique<ConstantBufferCache<TerrainCellDrawData>>(_context.get());

        _terrainHighLod = {};
        core::utility::SubdivideGrid_Internal(32, _terrainHighLod);
        _terrainMid1Lod = {};
        core::utility::SubdivideGrid_Internal(16, _terrainMid1Lod);
        _terrainMid2Lod = {};
        core::utility::SubdivideGrid_Internal(8, _terrainMid2Lod);
        _terrainLowLod = {};
        core::utility::SubdivideGrid_Internal(4, _terrainLowLod);
	}

	void Renderer::RenderTerrain(model::WorldModel* pWorld)
	{
		static bool doWireframe = false;
		{
			ImGui::Begin("Terrain render pass settings");
			ImGui::Checkbox("Wireframe", &doWireframe);
			if (ImGui::Button("Calculate all terrain LODs (WARNING EXPENSIVE)"))
			{
				for (auto& cell : pWorld->cellStore)
				{
					/*EnsureMeshLoaded(cell.highLod);
					EnsureMeshLoaded(cell.mid2Lod);
					EnsureMeshLoaded(cell.midLod);
					EnsureMeshLoaded(cell.lowLod);*/
				}
			}
			ImGui::End();
		}

		if (pWorld->terrainHeightMap.heightMapDirty)
		{
			RebuildHeightmap(&pWorld->terrainHeightMap);
		}

		if (pWorld->terrainHeightMap.overlayDirty)
		{
			RebuildHeightmapOverlay(&pWorld->terrainHeightMap);
		}

		if (pWorld->terrainHeightMap.splatDirty)
		{
			RebuildCellSplatMap(&pWorld->terrainHeightMap);
		}

		auto vertexShader = _shaderCache->GetShader("TerrainVertexShader.cso", dm3d::ShaderStage::Vertex);
		auto pixelShader = _shaderCache->GetShader("TerrainPixelShader.cso", dm3d::ShaderStage::Pixel);

		auto camera = std::dynamic_pointer_cast<core::Camera>(pWorld->globalObjectStore[pWorld->activeCamera]);
		auto viewMatrix = camera->GetViewMatrix();
		auto projMatrix = camera->GetProjectionMatrix();
		auto viewProj = projMatrix * viewMatrix;

		SceneData sceneData{ .vp = viewProj };
		auto sceneDataBuffer = _context->build_constant(sceneData, false);

		auto cmd = _context->allocate_command_list();

		// Get a reference to the current back buffer
		auto backBuffer = _context->get_back_buffer();

		// Transition used resources to the correct states
		cmd->try_defer_transition(backBuffer, dm3d::ResourceState::RenderTarget);
		cmd->try_defer_transition(_depthBuffer, dm3d::ResourceState::DepthWrite);

		// Setup GPU state, including render target
		cmd->bind_render_target(0, backBuffer);
		cmd->set_num_render_targets(1);
		cmd->bind_depth_target(_depthBuffer);
		cmd->set_fill_mode(doWireframe ? dm3d::Wireframe : dm3d::Solid);
		cmd->set_input_primitive(dm3d::TriangleList);
		cmd->set_primitive(dm3d::Triangle);
		cmd->set_cull_mode(dm3d::CullBack);
		cmd->set_depth_test_enabled(true);
		cmd->set_draw_extent(_context->get_current_draw_extent());

		cmd->set_vertex(vertexShader);
		cmd->set_pixel(pixelShader);

		auto camPos = camera->position;
		camPos.y = 0.f;

		for (auto& cell : pWorld->cellStore)
		{
			std::shared_ptr<dm3d::Buffer> vertexBuffer;
			std::shared_ptr<dm3d::IndexBuffer> indexBuffer;
			uint32_t indexCount;

			auto dist = glm::distance(camPos, cell.GetCenter());

            auto loadMesh = [&](core::MeshRenderable& renderable)
                {
                    EnsureMeshLoaded(renderable);
                    vertexBuffer = renderable.vertexBuffer;
                    indexBuffer = renderable.indexBuffer;
                    indexCount = renderable.numIndices;
                };

			if (dist < 256.f)
			{
                loadMesh(_terrainHighLod);
			}
			else if (dist < 1024.f)
			{
                loadMesh(_terrainMid1Lod);
			}
			else if (dist < 2048.f)
			{
                loadMesh(_terrainMid2Lod);
			}
			else
			{
                loadMesh(_terrainLowLod);
			}

			auto splatPackOpt = GetSplatPack(cell);

			if (!splatPackOpt.has_value())
				continue;

			auto& splatPack = splatPackOpt.value();

			TerrainCellDrawData cellDrawData = { .cellCenter = cell.GetCenter(), .pSplatTexture1 = splatPack.texture1->get_structured_index(), .pSplatTexture2 = splatPack.texture2->get_structured_index(), .pSplatTexture3 = splatPack.texture3->get_structured_index(), .pSplatTexture4 = splatPack.texture4->get_structured_index() };
            auto drawDataBuffer = _terrainDrawDataCache->Allocate();
			{
                auto pDrawData = drawDataBuffer->map();
                memcpy(pDrawData, &cellDrawData, sizeof(TerrainCellDrawData));
                drawDataBuffer->unmap();
			}

			TerrainResourceTable resourceTable{ .pVertexBuffer = vertexBuffer->get_structured_index(), .pSceneData = sceneDataBuffer->get_constant_index(), .pHeightMap = _heightMap->get_structured_index(), .pCellDrawData = drawDataBuffer->get_constant_index(), .pHeightMapOverlay = _heightMapOverlay->get_structured_index(), .pSplatMap = _heightMapSplat->get_structured_index() };
			
			cmd->try_defer_transition(vertexBuffer, dm3d::ResourceState::ShaderRead);

			cmd->set_resource_table(0, resourceTable);
			cmd->bind_index_buffer(indexBuffer);

			cmd->draw_indexed(indexCount, 0);
		}

		_context->submit_list(std::move(cmd));
	}

	void Renderer::RebuildHeightmap(model::TerrainHeightMap* pHeightMap)
	{
		auto heightMapWidth = pHeightMap->GetWidth();
		auto& floatData = pHeightMap->GetFloatData();
		std::vector<float> flatData(heightMapWidth * heightMapWidth);

		for (size_t y = 0; y < heightMapWidth; y++)
		{
			memcpy(&flatData[y * heightMapWidth], floatData[y].data(), heightMapWidth * sizeof(float));
		}

		_heightMap = _context->create_image(dm3d::Extent3D{ .width = static_cast<uint32_t>(heightMapWidth), .height = static_cast
			                                    <uint32_t>(heightMapWidth), .depth = 1 }, dm3d::R32_FLOAT, dm3d::None, dm3d::ResourceState::ShaderRead, "HeightMap");
		_context->register_image_view(_heightMap);

		_context->copy_image(flatData.data(), _heightMap);

		pHeightMap->heightMapDirty = false;
	}

	void Renderer::RebuildHeightmapOverlay(model::TerrainHeightMap* pHeightMap)
	{
		auto heightMapWidth = pHeightMap->GetWidth();
		auto& rawData = pHeightMap->GetOverlayData();
		std::vector<DMR8G8B8A8Pixel> flatData(heightMapWidth * heightMapWidth);

		for (size_t y = 0; y < heightMapWidth; y++)
		{
			memcpy(&flatData[y * heightMapWidth], rawData[y].data(), heightMapWidth * sizeof(DMR8G8B8A8Pixel));
		}

		_heightMapOverlay = _context->create_image(dm3d::Extent3D{ .width = static_cast<uint32_t>(heightMapWidth), .height = static_cast<uint32_t>(heightMapWidth), .depth = 1 },
			dm3d::R8G8B8A8_UNORM);
		_context->register_image_view(_heightMapOverlay);
		_context->copy_image(flatData.data(), _heightMapOverlay);

		pHeightMap->overlayDirty = false;
	}

	void Renderer::RebuildCellSplatMap(model::TerrainHeightMap* pHeightMap)
	{
		auto& rawData = pHeightMap->GetSplatData();
		auto splatWidth = rawData.size();
		std::vector<DMR8G8B8A8Pixel> flatData(splatWidth * splatWidth);

		for (size_t y = 0; y < splatWidth; y++)
		{
			memcpy(&flatData[y * splatWidth], rawData[y].data(), splatWidth * sizeof(DMR8G8B8A8Pixel));
		}

		_heightMapSplat = _context->create_image(dm3d::Extent3D{ .width = static_cast<uint32_t>(splatWidth), .height = static_cast<uint32_t>(splatWidth), .depth = 1 }, dm3d::R8G8B8A8_UNORM);
		_context->register_image_view(_heightMapSplat);
		_context->copy_image(flatData.data(), _heightMapSplat);

		pHeightMap->splatDirty = false;
	}

	std::optional<Renderer::SplatPack> Renderer::GetSplatPack(const model::Cell& pCell) const
	{
		auto texture1 = _assetManager->TryGetImage(pCell.GetTerrainTexture(0), true);
		auto texture2 = _assetManager->TryGetImage(pCell.GetTerrainTexture(1), true);
		auto texture3 = _assetManager->TryGetImage(pCell.GetTerrainTexture(2), true);
		auto texture4 = _assetManager->TryGetImage(pCell.GetTerrainTexture(3), true);

		if (texture1 == nullptr || texture2 == nullptr || texture3 == nullptr || texture4 == nullptr)
		{
			return std::optional<Renderer::SplatPack>();
		}

		return SplatPack{ .texture1 = texture1, .texture2 = texture2, .texture3 = texture3, .texture4 = texture4 };
	}

}
