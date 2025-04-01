#ifndef SHADER_TYPES_HLSI
#define SHADER_TYPES_HLSI

struct TerrainVertexOut
{
    float4 position : SV_Position;
    float2 heightMapUv : TEXCOORD1;
};

struct FullQuadVertexOut
{
    float2 texCoord : TEXCOORD0;
    float4 position : SV_Position;
};

#endif