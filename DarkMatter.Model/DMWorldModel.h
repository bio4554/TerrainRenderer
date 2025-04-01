#pragma once
#include <vector>

#include "DMCell.h"
#include "DMAssetRegistry.h"
#include "DMHeightMap.h"

namespace dm::model
{
	class WorldModel
	{
	public:
		TerrainHeightMap terrainHeightMap = TerrainHeightMap(0, 0);
		core::AssetRegistry assetRegistry;
		std::vector<Cell> cellRegistry;
		std::vector<Cell> cellStore;
		std::vector<std::shared_ptr<core::GameObject>> globalObjectStore;
		size_t activeCamera;
	};
}
