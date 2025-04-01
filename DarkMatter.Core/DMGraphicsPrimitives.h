#ifndef GRAPHICS_PRIMITIVES_H
#define GRAPHICS_PRIMITIVES_H

#include "DMHLSL_in_CPP.h"

#define DMVertex_Flag_None 0
#define DMVertex_Flag_NoHeightMap 1

// C++ only
#ifdef __cplusplus
struct DMR8G8B8A8Pixel
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};
#endif

struct DMVertex
{
	float3 position;
	float2 uvChannel1;
	uint flags;
};

#endif