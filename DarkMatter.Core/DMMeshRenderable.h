#pragma once

#include <vector>

#include "DM3DResource.h"
#include "DMGraphicsPrimitives.h"

namespace dm::core
{
	class MeshRenderable
	{
	public:
		std::vector<DMVertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t numIndices;

		std::shared_ptr<dm3d::Buffer> vertexBuffer = nullptr;
		std::shared_ptr<dm3d::IndexBuffer> indexBuffer = nullptr;
	};
}
