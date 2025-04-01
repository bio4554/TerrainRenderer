#include "pch.h"
#include "DMHeightMap.h"

namespace dm::model
{
	TerrainHeightMap::TerrainHeightMap(size_t width, size_t splatWidth)
	{
        _width = width;
        _splatWidth = splatWidth;
		_heightMap = std::vector<std::vector<float>>(width);
        _heightMapOverlay = std::vector<std::vector<DMR8G8B8A8Pixel>>(width);
        _heightMapSplat = std::vector<std::vector<DMR8G8B8A8Pixel>>(splatWidth);

		for (size_t i = 0; i < width; i++)
		{
            _heightMap[i] = std::vector<float>(width);
            _heightMapOverlay[i] = std::vector<DMR8G8B8A8Pixel>(width);
		}

        for (size_t i = 0; i < splatWidth; i++)
        {
            _heightMapSplat[i] = std::vector<DMR8G8B8A8Pixel>(splatWidth);

            for (size_t j = 0; j < splatWidth; j++)
            {
                _heightMapSplat[i][j] = DMR8G8B8A8Pixel{ .r = 255, .g = 0, .b = 0, .a = 0 };
            }
        }
	}

	std::vector<std::vector<float>>& TerrainHeightMap::GetFloatData()
	{
		return _heightMap;
	}

    std::vector<std::vector<DMR8G8B8A8Pixel>>& TerrainHeightMap::GetOverlayData()
    {
        return _heightMapOverlay;
    }

    std::vector<std::vector<DMR8G8B8A8Pixel>>& TerrainHeightMap::GetSplatData()
    {
        return _heightMapSplat;
    }

    void TerrainHeightMap::ClearOverlay(DMR8G8B8A8Pixel color)
    {
        // todo might be a faster way to clear this
        for (auto& row : _heightMapOverlay)
        {
	        for (auto& p : row)
	        {
                p = color;
	        }
        }
    }

	float TerrainHeightMap::GetHeight(float x, float z) const
	{
        // Clamp world coordinates to [0, 5120]
        x = (x < 0.0f) ? 0.0f : (x > 5120.0f ? 5120.0f : x);
        z = (z < 0.0f) ? 0.0f : (z > 5120.0f ? 5120.0f : z);

        // Convert to normalized texture coordinates [0, 1]
        float u = x / 5120.0f;
        float v = z / 5120.0f;

        // Map to texture pixel coordinates
        float texX = u * (1024 - 1);
        float texZ = v * (1024 - 1);

        // Calculate integer positions
        int x0 = static_cast<int>(texX);
        int x1 = (x0 < 1023) ? x0 + 1 : x0;
        int z0 = static_cast<int>(texZ);
        int z1 = (z0 < 1023) ? z0 + 1 : z0;

        // Fractional parts for interpolation
        float fracX = texX - x0;
        float fracZ = texZ - z0;

        // Fetch the height values from the texture data
        float h00 = _heightMap[z0][x0];
        float h10 = _heightMap[z0][x1];
        float h01 = _heightMap[z1][x0];
        float h11 = _heightMap[z1][x1];

        // Bilinear interpolation
        float h0 = h00 * (1.0f - fracX) + h10 * fracX;
        float h1 = h01 * (1.0f - fracX) + h11 * fracX;
        float height = h0 * (1.0f - fracZ) + h1 * fracZ;

        return height;
	}
}