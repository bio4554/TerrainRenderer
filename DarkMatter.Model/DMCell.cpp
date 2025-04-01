#include "pch.h"
#include "DMCell.h"

namespace dm::model
{
	Cell::Cell(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec2 uvTopLeft, glm::vec2 uvTopRight, glm::vec2 uvBottomLeft, glm::vec2 uvBottomRight, size_t splatWidth)
	{
		_extents[0] = topLeft;
		_extents[1] = topRight;
		_extents[2] = bottomLeft;
		_extents[3] = bottomRight;

		_heightmapCoordinates[0] = uvTopLeft;
		_heightmapCoordinates[1] = uvTopRight;
		_heightmapCoordinates[2] = uvBottomLeft;
		_heightmapCoordinates[3] = uvBottomRight;

        _center = (_extents[0] + _extents[1] + _extents[2] + _extents[3]) / 4.f;

		memset(_textures, 0, sizeof(_textures));
	}
}