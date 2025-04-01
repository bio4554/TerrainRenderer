#include "ShaderTypes.hlsli"

FullQuadVertexOut main(uint vertexIndex : SV_VertexID)
{
    FullQuadVertexOut output;

    float2 uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
    output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    output.texCoord = uv;
    return output;
}