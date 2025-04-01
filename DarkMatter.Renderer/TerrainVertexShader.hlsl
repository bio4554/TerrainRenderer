#include "SharedShaderTypes.h"
#include "ShaderTypes.hlsli"
#include "DMGraphicsPrimitives.h"
#include "GlobalBinding.hlsli"

ConstantBuffer<TerrainResourceTable> resources : register(b0);

TerrainVertexOut main(uint vertexId : SV_VertexID)
{
	ConstantBuffer<SceneData> sceneData = ResourceDescriptorHeap[resources.pSceneData];
	ConstantBuffer<TerrainCellDrawData> cellDrawData = ResourceDescriptorHeap[resources.pCellDrawData];
	StructuredBuffer<DMVertex> vertexBuffer = ResourceDescriptorHeap[resources.pVertexBuffer];

	DMVertex v = vertexBuffer[vertexId];

	Texture2D heightMap = ResourceDescriptorHeap[resources.pHeightMap];

	float4 worldPos = float4(v.position + cellDrawData.cellCenter, 1);

	float2 sampleUV = worldPos.xz / 5120.f;

	float4 heightSample = heightMap.SampleLevel(heightSampler, sampleUV, 0);

	TerrainVertexOut output;
	if (((v.flags & DMVertex_Flag_NoHeightMap) == DMVertex_Flag_NoHeightMap))
	{
		output.position = mul(sceneData.vp, worldPos);
	}
	else
	{
		output.position = mul(sceneData.vp, float4(worldPos.x, heightSample.x, worldPos.z, 1));
	}
	output.heightMapUv = sampleUV;

	return output;
}
