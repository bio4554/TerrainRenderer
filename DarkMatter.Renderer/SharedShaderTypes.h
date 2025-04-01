#ifndef SHARED_SHADER_TYPES_H
#define SHARED_SHADER_TYPES_H

#include "HLSL_in_CPP.h"

struct SceneData
{
	float4x4 vp;
};

struct TerrainCellDrawData
{
	float3 cellCenter;
	uint pSplatTexture1;
	uint pSplatTexture2;
	uint pSplatTexture3;
	uint pSplatTexture4;
};

struct TerrainResourceTable
{
	uint pVertexBuffer;
	uint pSceneData;
	uint pHeightMap;
	uint pCellDrawData;
	uint pHeightMapOverlay;
	uint pSplatMap;
};

#endif
