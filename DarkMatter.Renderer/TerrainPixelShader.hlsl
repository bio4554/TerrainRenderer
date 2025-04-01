#include "SharedShaderTypes.h"
#include "ShaderTypes.hlsli"
#include "DMGraphicsPrimitives.h"
#include "GlobalBinding.hlsli"

ConstantBuffer<TerrainResourceTable> resources : register(b0);

float4 hash4(float2 p)
{
    float4 d = float4(
        1.0 + dot(p, float2(37.0, 17.0)),
        2.0 + dot(p, float2(11.0, 47.0)),
        3.0 + dot(p, float2(41.0, 29.0)),
        4.0 + dot(p, float2(23.0, 31.0))
    );
    return frac(sin(d) * 103.0);
}

float4 UvFuck(float2 uv, Texture2D tex)
{
    int2 iuv = int2(floor(uv));
    float2 fuv = frac(uv);

    float4 ofa = hash4(iuv + int2(0, 0));
    float4 ofb = hash4(iuv + int2(1, 0));
    float4 ofc = hash4(iuv + int2(0, 1));
    float4 ofd = hash4(iuv + int2(1, 1));

    float2 ddx_uv = ddx(uv);
    float2 ddy_uv = ddy(uv);

    ofa.zw = sign(ofa.zw - 0.5);
    ofb.zw = sign(ofb.zw - 0.5);
    ofc.zw = sign(ofc.zw - 0.5);
    ofd.zw = sign(ofd.zw - 0.5);

    float2 uva = uv * ofa.zw + ofa.xy;
    float2 ddxa = ddx_uv * ofa.zw;
    float2 ddya = ddy_uv * ofa.zw;

    float2 uvb = uv * ofb.zw + ofb.xy;
    float2 ddxb = ddx_uv * ofb.zw;
    float2 ddyb = ddy_uv * ofb.zw;

    float2 uvc = uv * ofc.zw + ofc.xy;
    float2 ddxc = ddx_uv * ofc.zw;
    float2 ddyc = ddy_uv * ofc.zw;

    float2 uvd = uv * ofd.zw + ofd.xy;
    float2 ddxd = ddx_uv * ofd.zw;
    float2 ddyd = ddy_uv * ofd.zw;

    float2 b = smoothstep(0.25, 0.75, fuv);

    float4 sampleA = tex.SampleGrad(linearSampler, uva, ddxa, ddya);
    float4 sampleB = tex.SampleGrad(linearSampler, uvb, ddxb, ddyb);
    float4 sampleC = tex.SampleGrad(linearSampler, uvc, ddxc, ddyc);
    float4 sampleD = tex.SampleGrad(linearSampler, uvd, ddxd, ddyd);

    float4 mixAB = lerp(sampleA, sampleB, b.x);
    float4 mixCD = lerp(sampleC, sampleD, b.x);
    return lerp(mixAB, mixCD, b.y);
}

float3 ComputeNormal(float2 uv)
{
    float texelSize = 1.0 / 1024.0;
    Texture2D heightMap = ResourceDescriptorHeap[resources.pHeightMap];
    float heightL = heightMap.SampleLevel(heightSampler, uv - float2(texelSize, 0), 0).r;
    float heightR = heightMap.SampleLevel(heightSampler, uv + float2(texelSize, 0), 0).r;
    float heightD = heightMap.SampleLevel(heightSampler, uv - float2(0, texelSize), 0).r;
    float heightU = heightMap.SampleLevel(heightSampler, uv + float2(0, texelSize), 0).r;

    float dX = heightR - heightL;
    float dZ = heightU - heightD;

    float3 normal = normalize(float3(-dX, 2.0, -dZ));
    return normal;
}

float4 GetSplatColor(float2 uv)
{
    Texture2D splatMap = ResourceDescriptorHeap[resources.pSplatMap];
    ConstantBuffer<TerrainCellDrawData> drawData = ResourceDescriptorHeap[resources.pCellDrawData];
    Texture2D splat1 = ResourceDescriptorHeap[drawData.pSplatTexture1];
    Texture2D splat2 = ResourceDescriptorHeap[drawData.pSplatTexture1];
    Texture2D splat3 = ResourceDescriptorHeap[drawData.pSplatTexture1];
    Texture2D splat4 = ResourceDescriptorHeap[drawData.pSplatTexture1];

    float4 splatSample = splatMap.Sample(linearSampler, uv);
    float2 scaledUv = uv * 1000;
    float3 finalColor = float3(0, 0, 0);
    finalColor = finalColor + ((UvFuck(scaledUv, splat1) * splatSample.x).xyz);
    finalColor = finalColor + ((UvFuck(scaledUv, splat2) * splatSample.y).xyz);
    finalColor = finalColor + ((UvFuck(scaledUv, splat3) * splatSample.z).xyz);
    finalColor = finalColor + ((UvFuck(scaledUv, splat4) * splatSample.w).xyz);
    return float4(finalColor, 1);
}

float4 main(TerrainVertexOut input) : SV_TARGET
{
    Texture2D heightMapOverlay = ResourceDescriptorHeap[resources.pHeightMapOverlay];

    float4 overlayColor = heightMapOverlay.Sample(linearSampler, input.heightMapUv);

    float4 splatColor = GetSplatColor(input.heightMapUv);

    float3 normal = ComputeNormal(input.heightMapUv);

    float4 finalColor = splatColor + overlayColor;

    float3 lightDir = normalize(float3(0.f, -1.f, 1.f));

    float lightDiff = max(dot(normal, -lightDir), 0.0);

    return lightDiff * finalColor;
}