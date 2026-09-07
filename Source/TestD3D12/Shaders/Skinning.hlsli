#ifndef __SKINNING_HLSLI__
#define __SKINNING_HLSLI__

#include "ShaderInterop.h"

#if GPU_SKINNING

#define ROOT_SIGNATURE_SKINNING_RESOURCES "SRV(t1, visibility=SHADER_VISIBILITY_VERTEX), "

// All instances in the draw share this buffer; each surface supplies its joint element offset.
StructuredBuffer<ShaderSkinningJoint> skinningJointBuffer : register(t1);

struct SkinningVertexInput {
#if GPU_SKINNING_WEIGHTS == 1
    uint jointIndex : BLENDINDICES0;
#elif GPU_SKINNING_WEIGHTS == 4 || GPU_SKINNING_WEIGHTS == 8
    uint4 jointIndexes0 : BLENDINDICES0;
    float4 jointWeights0 : BLENDWEIGHT0;
#if GPU_SKINNING_WEIGHTS == 8
    uint4 jointIndexes1 : BLENDINDICES1;
    float4 jointWeights1 : BLENDWEIGHT1;
#endif
#else
    #error Unsupported GPU_SKINNING_WEIGHTS
#endif
};

float3x4 GetSkinningMatrix(SkinningVertexInput input, uint jointOffset) {
#if GPU_SKINNING_WEIGHTS == 1
    return skinningJointBuffer[jointOffset + input.jointIndex].matrix;
#else
    // UNORM vertex weights are already converted to [0, 1] by the input layout.
#if GPU_SKINNING_WEIGHTS == 4
    [branch]
    if (all(input.jointWeights0 == float4(1.0f, 0.0f, 0.0f, 0.0f))) {
        return skinningJointBuffer[jointOffset + input.jointIndexes0.x].matrix;
    }
#endif

    float3x4 skinningMatrix =
        skinningJointBuffer[jointOffset + input.jointIndexes0.x].matrix * input.jointWeights0.x +
        skinningJointBuffer[jointOffset + input.jointIndexes0.y].matrix * input.jointWeights0.y +
        skinningJointBuffer[jointOffset + input.jointIndexes0.z].matrix * input.jointWeights0.z +
        skinningJointBuffer[jointOffset + input.jointIndexes0.w].matrix * input.jointWeights0.w;
#if GPU_SKINNING_WEIGHTS == 8
    skinningMatrix +=
        skinningJointBuffer[jointOffset + input.jointIndexes1.x].matrix * input.jointWeights1.x +
        skinningJointBuffer[jointOffset + input.jointIndexes1.y].matrix * input.jointWeights1.y +
        skinningJointBuffer[jointOffset + input.jointIndexes1.z].matrix * input.jointWeights1.z +
        skinningJointBuffer[jointOffset + input.jointIndexes1.w].matrix * input.jointWeights1.w;
#endif
    return skinningMatrix;
#endif
}

#ifdef OUTPUT_VELOCITY
float4 GetPreviousSkinnedPosition(SkinningVertexInput input, float4 bindPosition, float4 currentPosition, uint jointOffset, uint prevJointOffset) {
    // History resets alias the current pose and need no additional joint reads.
    if (prevJointOffset == jointOffset) {
        return currentPosition;
    }
    return float4(mul(GetSkinningMatrix(input, prevJointOffset), bindPosition), 1.0f);
}
#endif

#else // GPU_SKINNING

#define ROOT_SIGNATURE_SKINNING_RESOURCES ""

#endif // GPU_SKINNING

#endif // __SKINNING_HLSLI__
