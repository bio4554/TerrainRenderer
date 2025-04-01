#pragma once
#include <cstdint>

namespace dm3d
{
	struct Extent2D
	{
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct Extent3D
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	struct DarkMatter3DUsageStats
	{
		uint64_t availableMemory;
		uint64_t usedMemory;
		uint32_t allocators;
		uint32_t usedAllocators;
		uint32_t allocatedResources;
		int32_t frameAllocatedResources;
	};
}
