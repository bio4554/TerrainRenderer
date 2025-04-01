#ifndef GLOBAL_BINDING_HLSLI
#define GLOBAL_BINDING_HLSLI

SamplerState linearSampler : register(s0);
SamplerState pcfSampler : register(s1);
SamplerState pointSampler : register(s2);
SamplerState heightSampler : register(s3);

#endif