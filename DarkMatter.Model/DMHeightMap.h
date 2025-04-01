#pragma once
#include <vector>

#include "DMGraphicsPrimitives.h"

namespace dm::model
{
	class TerrainHeightMap
	{
	public:
		TerrainHeightMap(size_t width, size_t splatWidth);

		std::vector<std::vector<float>>& GetFloatData();
		std::vector<std::vector<DMR8G8B8A8Pixel>>& GetOverlayData();
		std::vector<std::vector<DMR8G8B8A8Pixel>>& GetSplatData();
		void ClearOverlay(DMR8G8B8A8Pixel color);
		float GetHeight(float x, float z) const;
		size_t GetWidth() const { return _width; }
		size_t GetSplatWidth() const { return _splatWidth; }

		bool heightMapDirty = true;
		bool overlayDirty = true;
		bool splatDirty = true;
	private:
		size_t _width, _splatWidth;
		std::vector<std::vector<float>> _heightMap;
		std::vector<std::vector<DMR8G8B8A8Pixel>> _heightMapOverlay;
		std::vector<std::vector<DMR8G8B8A8Pixel>> _heightMapSplat;
	};
}
