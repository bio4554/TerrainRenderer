#pragma once

#include <glm/vec2.hpp>

#include "glm/vec3.hpp"
#include "DMContainer.h"
#include "DMGraphicsPrimitives.h"
#include "DMMeshRenderable.h"

namespace dm::model
{
	class Cell : public core::Container
	{
	public:
		Cell(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec2 uvTopLeft, glm::vec2 uvTopRight, glm::vec2 uvBottomLeft, glm::vec2 uvBottomRight, size_t splatWidth);

		glm::vec3 GetTopLeft() const { return _extents[0]; }
		glm::vec3 GetTopRight() const { return _extents[1]; }
		glm::vec3 GetBottomLeft() const { return _extents[2]; }
		glm::vec3 GetBottomRight() const { return _extents[3]; }

		glm::vec2 GetUVTopLeft() const { return _heightmapCoordinates[0]; }
		glm::vec2 GetUVTopRight() const { return _heightmapCoordinates[1]; }
		glm::vec2 GetUVBottomLeft() const { return _heightmapCoordinates[2]; }
		glm::vec2 GetUVBottomRight() const { return _heightmapCoordinates[3]; }

		uint32_t GetTerrainTexture(uint32_t textureIdx) const { return _textures[textureIdx]; }
		void SetTerrainTexture(uint32_t textureIdx, uint32_t textureId) { _textures[textureIdx] = textureId; }

		glm::vec3 GetCenter() const { return _center; }

		std::string& GetName() { return _name; }

	private:
		uint32_t _textures[4];

		glm::vec3 _center;
		glm::vec3 _extents[4];
		glm::vec2 _heightmapCoordinates[4];

		std::string _name = "Cell";
	};
}
