#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../PBR/ImportanceSampling.hlsli"
#include "../NormalEncoding.hlsli"

// Depth becomes writable again for opaque rendering after the AO passes.
// Opaque shading may overwrite this prepass normal after SSAO.
// Keep descriptor storage static, but do not promise static resource data.
#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0, flags = DATA_VOLATILE), SRV(t1, flags = DATA_VOLATILE), UAV(u0)), " \
    "RootConstants(num32BitConstants=12, b0), " \
    "CBV(b1)"

#define HALF_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0, flags = DATA_VOLATILE), SRV(t1, flags = DATA_VOLATILE), UAV(u0), UAV(u1)), " \
    "RootConstants(num32BitConstants=12, b0), " \
    "CBV(b1)"

cbuffer SSAOCB : register(b0) {
    SSAOParams ssaoParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputDepthTexture : register(t0);
Texture2D<float2> inputNormalTexture : register(t1);
RWTexture2D<float> outputAOTexture : register(u0);
RWTexture2D<float> outputDepthTexture : register(u1);

#ifndef SSAO_SAMPLE_COUNT
#define SSAO_SAMPLE_COUNT 16
#endif

static const uint ssaoSampleCount = SSAO_SAMPLE_COUNT;

#if SSAO_SAMPLE_COUNT == 8
// Hammersley(i, 8) + float2(0.5 / 8, 0) 로 반구 전체를 덮는 전용 cosine-weighted 커널.
// 방향과 거리의 상관을 줄이도록 radius 의 순서를 { 3,7,1,5,2,6,0,4 } 로 섞는다.
// W = lerp(0.05, 1, u*u), u = (permutedIndex + 0.5) / 8.
static const float4 ssaoKernel[ssaoSampleCount] = {
    float4(0.250000000f, 0.000000000f, 0.968245837f, 0.231835937f),
    float4(-0.433012702f, 0.000000000f, 0.901387819f, 0.884960938f),
    float4(0.000000000f, 0.559016994f, 0.829156198f, 0.083398438f),
    float4(0.000000000f, -0.661437828f, 0.750000000f, 0.499023437f),
    float4(0.530330086f, 0.530330086f, 0.661437828f, 0.142773437f),
    float4(-0.586301970f, -0.586301970f, 0.559016994f, 0.677148437f),
    float4(-0.637377439f, 0.637377439f, 0.433012702f, 0.053710938f),
    float4(0.684653197f, -0.684653197f, 0.250000000f, 0.350585938f)
};
#elif SSAO_SAMPLE_COUNT == 16
// Precomputed SampleHemisphereCosine(Hammersley(i, 16) + float2(0.5 / 16, 0)).
// W = lerp(0.05, 1, u*u), u = (permutedIndex + 0.5) / 16. The independent
// permutation is { 7,14,2,10,5,12,0,9,15,4,11,1,13,6,3,8 }.
// Keeping directions and radii constant avoids per-sample trigonometry or uploads.
static const float4 ssaoKernel[ssaoSampleCount] = {
    float4(0.176776695f, 0.000000000f, 0.984250984f, 0.258740234f),
    float4(-0.306186218f, 0.000000000f, 0.951971638f, 0.830224609f),
    float4(0.000000000f, 0.395284708f, 0.918558654f, 0.073193359f),
    float4(0.000000000f, -0.467707173f, 0.883883476f, 0.459130859f),
    float4(0.375000000f, 0.375000000f, 0.847791248f, 0.162255859f),
    float4(-0.414578099f, -0.414578099f, 0.810092587f, 0.629833984f),
    float4(-0.450693909f, 0.450693909f, 0.770551750f, 0.050927734f),
    float4(0.484122918f, -0.484122918f, 0.728868987f, 0.384912109f),
    float4(0.673387139f, 0.278926086f, 0.684653197f, 0.941552734f),
    float4(-0.711896991f, -0.294877389f, 0.637377439f, 0.125146484f),
    float4(-0.310009012f, 0.748427961f, 0.586301970f, 0.540771484f),
    float4(0.324435665f, -0.783256982f, 0.530330086f, 0.058349609f),
    float4(0.338247563f, 0.816601853f, 0.467707173f, 0.726318359f),
    float4(-0.351517178f, -0.848637539f, 0.395284708f, 0.206787109f),
    float4(-0.879507112f, 0.364303774f, 0.306186218f, 0.095458984f),
    float4(0.909329339f, -0.376656545f, 0.176776695f, 0.318115234f)
};
#else
#error SSAO_SAMPLE_COUNT must be 8 or 16
#endif

bool IsBackgroundDepth(float depth) {
    // 원본 깊이와 min/max로 resolve한 깊이는 clear 값을 그대로 보존한다.
    // 원거리 표면을 배경으로 오인하지 않도록 clear 값만 제외한다.
    return depth == ssaoParams.farPlaneDeviceDepth;
}

float LinearDepth(float depth) {
    // 표면 위치를 복원하기 전에 호출부에서 배경 깊이를 제외한다.
    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    return ToLinearDepth(depth, cameraConstants.ndcDepthScaleBias, cameraConstants.projectionZ, orthogonal);
}

// Retained for depth-based normal reconstruction; SSAO uses prepass normals.
// Select a neighbor on each axis from the same surface where possible.
// Invalid/background neighbors must not participate in reconstruction, including
// infinite-far depth. Only the two selected neighbor positions are reconstructed.
bool ReconstructNormal(int2 inputPixelCoord, float3 viewPos, out float3 normal) {
    const int2 offsets[4] = {int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1)};
    float linearDepths[4];
    float errors[4];
    const float centerLinearDepth = -viewPos.z;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        const int2 neighbor = inputPixelCoord + offsets[i];
        const bool inBounds = all(neighbor >= 0) && all(neighbor < int2(ssaoParams.inputSize));
        const float depth = inBounds ? inputDepthTexture.Load(int3(neighbor, 0)) : ssaoParams.farPlaneDeviceDepth;
        const bool background = IsBackgroundDepth(depth);
        linearDepths[i] = background ? 0.0f : LinearDepth(depth);
        errors[i] = background ? FLT_MAX : abs(linearDepths[i] - centerLinearDepth);
    }

    const uint horizontal = errors[0] < errors[1] ? 0u : 1u;
    const uint vertical = errors[2] < errors[3] ? 2u : 3u;
    normal = 0.0f;

    if (errors[horizontal] == FLT_MAX || errors[vertical] == FLT_MAX) {
        return false;
    }

    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    const float2 horizontalUV = (float2(inputPixelCoord + offsets[horizontal]) + 0.5f) * ssaoParams.inputSizeRcp;
    const float2 verticalUV = (float2(inputPixelCoord + offsets[vertical]) + 0.5f) * ssaoParams.inputSizeRcp;
    const float3 dx = ViewPositionFromLinearDepth(horizontalUV, linearDepths[horizontal], cameraConstants.uvToView, orthogonal) - viewPos;
    const float3 dy = ViewPositionFromLinearDepth(verticalUV, linearDepths[vertical], cameraConstants.uvToView, orthogonal) - viewPos;
    normal = cross(dx, dy);
    const float lengthSquared = dot(normal, normal);
    if (lengthSquared <= 1.0e-20f) {
        return false;
    }
    normal *= rsqrt(lengthSquared);

    const float3 toCamera = orthogonal ? float3(0.0f, 0.0f, 1.0f) : -viewPos;
    normal *= dot(normal, toCamera) < 0.0f ? -1.0f : 1.0f;
    return true;
}

float EvaluateSSAO(uint2 outputPixelCoord, uint2 inputPixelCoord, float centerDepth) {
    if (IsBackgroundDepth(centerDepth)) {
        return 1.0f;
    }

    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    const float2 uv = (float2(inputPixelCoord) + 0.5f) * ssaoParams.inputSizeRcp;
    const float3 viewPos = ViewPositionFromLinearDepth(uv, LinearDepth(centerDepth), cameraConstants.uvToView, orthogonal);

    // Half resolution uses the normal of the same full-resolution pixel selected for depth.
    // The prepass already orients two-sided smooth normals by primitive facing.
    const float3 normal = DecodeOctahedralNormal(inputNormalTexture.Load(int3(inputPixelCoord, 0)));

    // IGN 으로 주변 픽셀의 샘플 회전각을 고르게 분포시킨다.
    // TAA 가 누적할 수 있도록 프레임별 위상을 더한다. noisePhase 가 0 이면 기존 패턴을 유지한다.
    const float angle = TWO_PI * frac(InterleavedGradientNoise(outputPixelCoord) + ssaoParams.noisePhase);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);
    
    const float3x3 frame = GetLocalFrame(normal);
    const float3 tangent = cosAngle * frame[0] + sinAngle * frame[1];
    const float3 bitangent = -sinAngle * frame[0] + cosAngle * frame[1];
    const float3 origin = viewPos + ssaoParams.normalBias * normal;
    const float inverseRadiusSquared = rcp(ssaoParams.radius * ssaoParams.radius);

    float occlusion = 0.0f;
    [unroll]
    for (uint sampleIndex = 0; sampleIndex < ssaoSampleCount; ++sampleIndex) {
        const float4 kernel = ssaoKernel[sampleIndex];
        const float3 direction = kernel.x * tangent + kernel.y * bitangent + kernel.z * normal;
        const float3 samplePosition = origin + ssaoParams.radius * kernel.w * direction;
        const float4 clipPosition = ProjectViewPosition(samplePosition, cameraConstants.projectionXY, cameraConstants.projectionZ, orthogonal);
        if (clipPosition.w <= 1.0e-5f) {
            continue;
        }

        const float3 screenPosition = ClipPositionToScreenPosition(clipPosition, cameraConstants.ndcDepthScaleBias);
        if (any(screenPosition.xy < 0.0f) || any(screenPosition.xy >= 1.0f) || screenPosition.z < 0.0f || screenPosition.z > 1.0f) {
            continue;
        }

        const uint2 samplePixel = min(uint2(screenPosition.xy * ssaoParams.inputSize), ssaoParams.inputSize - 1u);
        if (all(samplePixel == inputPixelCoord)) {
            continue;
        }

        const float sceneDepth = inputDepthTexture.Load(int3(samplePixel, 0));
        if (IsBackgroundDepth(sceneDepth)) {
            continue;
        }
        
        const float sceneLinearDepth = LinearDepth(sceneDepth);
        const float sampleLinearDepth = -samplePosition.z;
        if (sceneLinearDepth >= sampleLinearDepth - ssaoParams.depthBias) {
            continue;
        }

        // Reconstruct at the loaded texel center, not the projected sample UV.
        const float2 sampleUV = (float2(samplePixel) + 0.5f) * ssaoParams.inputSizeRcp;
        const float3 delta = ViewPositionFromLinearDepth(sampleUV, sceneLinearDepth, cameraConstants.uvToView, orthogonal) - viewPos;
        // Point sampling can place a sloped plane in front of Q. A point on the
        // receiver's tangent plane must not count as its own occluder.
        const float normalDistance = dot(normal, delta);
        if (normalDistance <= ssaoParams.normalBias) {
            continue;
        }

        const float distanceSquared = dot(delta, delta);
        const float weight = saturate(1.0f - distanceSquared * inverseRadiusSquared);
        occlusion += weight * weight;
    }

    // Cosine-weighted directions already account for N dot L. Offscreen and
    // rejected samples remain unoccluded; keep the full sample count divisor.
    float ao = saturate(1.0f - occlusion / float(ssaoSampleCount));
    // Apply power here for both filtered and unfiltered AO; filtering consumes this result.
    if (ssaoParams.power != 1.0f) {
        ao = pow(ao, ssaoParams.power);
    }
    return ao;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSAOBlockSizeX, SSAOBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= ssaoParams.outputSize)) {
        return;
    }
    outputAOTexture[pixelCoord] = EvaluateSSAO(pixelCoord, pixelCoord, inputDepthTexture.Load(int3(pixelCoord, 0)));
}

[RootSignature(HALF_ROOT_SIGNATURE)]
[numthreads(SSAOBlockSizeX, SSAOBlockSizeY, 1)]
void CSMainHalfResolution(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputPixelCoord = dispatchThreadId.xy;
    if (any(outputPixelCoord >= ssaoParams.outputSize)) {
        return;
    }

    // 2x2 블록의 가장 가까운 표면을 선택한다. 홀수 해상도의 마지막 블록은 유효 픽셀만 사용한다.
    const uint2 blockOrigin = outputPixelCoord * 2u;
    uint2 inputPixelCoord = blockOrigin;
    float centerDepth = ssaoParams.farPlaneDeviceDepth;
    const bool reverseZ = ssaoParams.farPlaneDeviceDepth == 0.0f;

    [unroll]
    for (uint y = 0; y < 2; ++y) {
        [unroll]
        for (uint x = 0; x < 2; ++x) {
            const uint2 sampleCoord = blockOrigin + uint2(x, y);
            if (all(sampleCoord < ssaoParams.inputSize)) {
                const float depth = inputDepthTexture.Load(int3(sampleCoord, 0));
                if (!IsBackgroundDepth(depth) && (IsBackgroundDepth(centerDepth) || (reverseZ ? depth > centerDepth : depth < centerDepth))) {
                    centerDepth = depth;
                    inputPixelCoord = sampleCoord;
                }
            }
        }
    }

    // AO 와 동일한 표면의 device depth 를 보존해 blur 와 upsampling 에서 공유한다.
    outputDepthTexture[outputPixelCoord] = centerDepth;
    outputAOTexture[outputPixelCoord] = EvaluateSSAO(outputPixelCoord, inputPixelCoord, centerDepth);
}
