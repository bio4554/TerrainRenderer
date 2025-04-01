#ifndef HLSL_IN_CPP_H
#define HLSL_IN_CPP_H

#ifdef __cplusplus
#include <glm/mat4x4.hpp>

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using uint = uint32_t;

using float4x4 = glm::mat4x4;
#endif

#endif