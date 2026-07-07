#include "Common.hlsli"

#ifdef USE_DIRECT_DESCRIPTOR_HEAP
#define ROOT_SIGNATURE_HIZ_CULL_ROOT_FLAGS "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), "
#else
#define ROOT_SIGNATURE_HIZ_CULL_ROOT_FLAGS "RootFlags(0), "
#endif

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_HIZ_CULL_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0), UAV(u1)), " \
    "CBV(b0), " \
    "CBV(b1)"

cbuffer HiZCullInstancesCB : register(b0) {
    HiZCullInstancesParams hiZCullParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

StructuredBuffer<uint> candidateInstanceBuffer : register(t0);
Texture2D<float> hiZTexture : register(t1);

RWStructuredBuffer<uint> visibleInstanceBuffer : register(u0);
RWByteAddressBuffer indirectArgsBuffer : register(u1);

// sphere radius 에 worldMatrix 의 non-uniform scale 적용
float GetWorldSphereRadius(float localRadius, row_major float3x4 worldMatrix) {
    const float sx = length(worldMatrix[0].xyz);
    const float sy = length(worldMatrix[1].xyz);
    const float sz = length(worldMatrix[2].xyz);
    return localRadius * max(max(sx, sy), sz);
}

bool ProjectSphereToScreenRect(float3 worldCenter, float worldRadius, out float4 screenRect, out float nearestDepth) {
    float2 rectMin = float2(1.0, 1.0);
    float2 rectMax = float2(0.0, 0.0);

    nearestDepth = hiZCullParams.useMinDepth != 0 ? 0.0 : 1.0;

    // sphere 를 감싸는 world-space AABB 의 8 corner 를 clip space 로 변환하여 screen rect 와 nearest depth 를 계산한다.
    [unroll]
    for (uint cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
        const float3 cornerOffset = float3(
            (cornerIndex & 1) != 0 ? worldRadius : -worldRadius,
            (cornerIndex & 2) != 0 ? worldRadius : -worldRadius,
            (cornerIndex & 4) != 0 ? worldRadius : -worldRadius);

        // Hi-Z texture 가 생성된 view-projection 기준으로 투영한다.
        const float4 clipPosition = mul(cameraConstants.hiZCullViewProjMatrix, float4(worldCenter + cornerOffset, 1.0));
        if (clipPosition.w <= 0.0) {
            // corner 가 하나라도 camera 뒤에 있는 경우 false 리턴
            screenRect = float4(0.0, 0.0, 1.0, 1.0);
            return false;
        }

        const float3 ndc = clipPosition.xyz / clipPosition.w;
        if (ndc.z < 0.0 || ndc.z > 1.0) {
            // corner 가 하나라도 depth clip range [0, 1] 밖이면 false 리턴
            screenRect = float4(0.0, 0.0, 1.0, 1.0);
            return false;
        }

        const float2 screenUV = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
        rectMin = min(rectMin, screenUV);
        rectMax = max(rectMax, screenUV);

        // nearest depth 는 hi-Z depth 비교를 위해 sphere 의 8 corner 중 가장 가까운 depth 를 선택한다.
        nearestDepth = hiZCullParams.useMinDepth != 0 ? max(nearestDepth, ndc.z) : min(nearestDepth, ndc.z);
    }

    screenRect = float4(saturate(rectMin), saturate(rectMax));
    return screenRect.x < screenRect.z && screenRect.y < screenRect.w;
}

// screenRect 는 [0, 1] 범위의 normalized screen space (xmin, ymin, xmax, ymax) 좌표
uint SelectHiZMip(float4 screenRect) {
    const float2 hiZSize = float2(hiZCullParams.hiZWidth, hiZCullParams.hiZHeight);
    const float2 rectSize = max((screenRect.zw - screenRect.xy) * hiZSize, float2(1.0, 1.0));
    const uint mipLevel = (uint)ceil(log2(max(rectSize.x, rectSize.y)));
    return min(mipLevel, hiZCullParams.hiZMipCount - 1);
}

bool IsOccluded(float4 screenRect, float nearestDepth) {
    const uint mipLevel = SelectHiZMip(screenRect);
    const uint2 mipSize = max(uint2(hiZCullParams.hiZWidth, hiZCullParams.hiZHeight) >> mipLevel, uint2(1, 1));

    // mip level 의 texel 좌표를 계산한다.
    const uint4 rect = min((uint4)(screenRect.xyzw * mipSize.xyxy), mipSize.xyxy - 1);

    // mip level 의 4 corner texel depth 를 샘플링한다. (texelMin, texelMax 가 texel 의 경계에 걸쳐있을 수 있으므로 4 corner 를 샘플링한다)
    const float d0 = hiZTexture.Load(uint3(rect.xy, mipLevel));
    const float d1 = hiZTexture.Load(uint3(rect.zy, mipLevel));
    const float d2 = hiZTexture.Load(uint3(rect.xw, mipLevel));
    const float d3 = hiZTexture.Load(uint3(rect.zw, mipLevel));

    // nearest depth 와 hi-Z depth 를 비교하여 occlusion 여부를 결정한다.
    const float hiZDepth = hiZCullParams.useMinDepth != 0 ? min(min(d0, d1), min(d2, d3)) : max(max(d0, d1), max(d2, d3));
    const float depthBias = 1.0e-5;
    return hiZCullParams.useMinDepth != 0 ? nearestDepth < hiZDepth - depthBias : nearestDepth > hiZDepth + depthBias;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(HiZCullThreadCount, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint instanceIndex = dispatchThreadId.x;
    if (instanceIndex >= hiZCullParams.inputInstanceCount) {
        return;
    }

    const uint shaderSurfaceIndex = candidateInstanceBuffer[instanceIndex];
    const ShaderSurface shaderSurface = GetShaderSurfaceTable(hiZCullParams.surfaceTableIndex)[shaderSurfaceIndex];

    bool visible = true;
    const float worldRadius = GetWorldSphereRadius(shaderSurface.localBoundingSphere.w, shaderSurface.worldMatrix);

    if (worldRadius > 0.0) {
        const float3 worldCenter = mul(shaderSurface.worldMatrix, float4(shaderSurface.localBoundingSphere.xyz, 1.0));

        // world bounding sphere 의 screen rect 와 nearest depth 를 계산한다.
        float4 screenRect;
        float nearestDepth;
        bool canTestOcclusion = ProjectSphereToScreenRect(worldCenter, worldRadius, screenRect, nearestDepth);
        if (canTestOcclusion) {
            visible = !IsOccluded(screenRect, nearestDepth);
        } else {
            // screen rect 계산에 실패하면 visible 을 true 로 설정한다. (카메라 뒤에 있거나 depth clip range 밖에 있는 경우)
            visible = true;
        }
    }

    if (visible) {
        uint instanceCount;
        // indirect batch 의 instance count 를 atomic add 로 증가시키고, visible instance buffer 에 shader surface index 를 채운다.
        indirectArgsBuffer.InterlockedAdd(hiZCullParams.indirectArgOffset + HiZCullIndirectArgInstanceCountOffset, 1, instanceCount);
        visibleInstanceBuffer[hiZCullParams.instanceBaseOffset + instanceCount] = shaderSurfaceIndex;
    }
}
