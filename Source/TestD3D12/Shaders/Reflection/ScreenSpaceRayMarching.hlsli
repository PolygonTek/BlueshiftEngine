#ifndef __SCREEN_SPACE_RAY_MARCHING_HLSLI__
#define __SCREEN_SPACE_RAY_MARCHING_HLSLI__

struct ScreenSpaceRayTraceResult {
    float2 hitUV;
    float hitDeviceDepth;
    uint status;
    uint iterationCount;
};

bool IsScreenSpaceRayInsideViewport(float2 uv) {
    return all(uv >= 0.0f) && all(uv < 1.0f);
}

// 현재 mip cell 에서 ray 진행 방향으로 가장 먼저 만나는 축 경계의 parameter t 를 구한다.
// 미리 계산한 inverseRayDelta 를 사용해 순회 중의 나눗셈을 피하며, 해당 축으로 이동하지 않는 경우에는 FLT_MAX 를 반환하여 다른 축의 경계가 선택되게 한다.
float FindScreenSpaceRayCellExit(float rayStart, float rayDelta, float inverseRayDelta, bool hasRayDelta, uint cellCoord, float currentT, float cellSize, float parameterEpsilon) {
    if (!hasRayDelta) {
        return FLT_MAX;
    }

    float boundary = float(rayDelta > 0.0f ? cellCoord + 1u : cellCoord) * cellSize;
    float exitT = (boundary - rayStart) * inverseRayDelta;

    if (exitT <= currentT + parameterEpsilon) {
        boundary += rayDelta > 0.0f ? cellSize : -cellSize;
        exitT = (boundary - rayStart) * inverseRayDelta;
    }
    return exitT;
}

// Ray 의 parameter t 위치에서 full-resolution scene depth 를 읽어 앞뒤 관계를 평가한다.
// Ray 가 viewport/device depth 범위 밖에 있거나 scene depth 가 clear 값이면 false 를 반환한다.
// depthDelta 는 standard/reverse Z 와 관계없이 음수는 surface 앞, 양수는 surface 뒤를 의미한다.
bool EvaluateScreenSpaceRayDeviceDepth(float3 rayStart, float3 rayDelta, float t, uint2 depthTextureSize, float farPlaneDeviceDepth, bool useMinDepth, out float depthDelta, out float sceneDeviceDepth) {
    const float3 rayPosition = rayStart + rayDelta * t;

    if (!IsScreenSpaceRayInsideViewport(rayPosition.xy) || rayPosition.z < 0.0f || rayPosition.z > 1.0f) {
        depthDelta = -FLT_MAX;
        sceneDeviceDepth = farPlaneDeviceDepth;
        return false;
    }

    const uint2 pixelCoord = min(uint2(rayPosition.xy * depthTextureSize), depthTextureSize - 1);
    sceneDeviceDepth = inputDepthTexture.Load(int3(pixelCoord, 0));
    if (abs(sceneDeviceDepth - farPlaneDeviceDepth) <= FLT_EPSILON) {
        depthDelta = -FLT_MAX;
        return false;
    }

    // Device depth 의 수치 방향을 정규화하여 음수는 ray 가 surface 앞에 있고,
    // 양수는 surface 뒤에 있음을 나타내도록 한다.
    depthDelta = useMinDepth ? rayPosition.z - sceneDeviceDepth : sceneDeviceDepth - rayPosition.z;
    return true;
}

// Mip 0 cell 의 full-resolution depth crossing 을 확인하고 binary refinement 와 thickness 검사를 거쳐 hit 를 확정한다.
bool TryRefineScreenSpaceRayHit(float3 rayStart, float3 rayDelta, float entryT, float exitT, uint2 depthTextureSize, uint binaryRefinementSteps,
    float thickness, float farPlaneDeviceDepth, bool useMinDepth, float2 ndcDepthScaleBias, float2 projectionDepthParams, bool orthogonal,
    inout ScreenSpaceRayTraceResult result) {
    float entryDepthDelta;
    float entrySceneDepth;
    const bool entryDepthValid = EvaluateScreenSpaceRayDeviceDepth(rayStart, rayDelta, entryT, depthTextureSize, farPlaneDeviceDepth, useMinDepth, entryDepthDelta, entrySceneDepth);

    float exitDepthDelta;
    float exitSceneDepth;
    const bool exitDepthValid = EvaluateScreenSpaceRayDeviceDepth(rayStart, rayDelta, exitT, depthTextureSize, farPlaneDeviceDepth, useMinDepth, exitDepthDelta, exitSceneDepth);

    if ((!entryDepthValid || entryDepthDelta <= 0.0f) && 
        (exitDepthValid && exitDepthDelta >= 0.0f)) {
        float frontT = entryT;
        float backT = exitT;
        float backSceneDeviceDepth = exitSceneDepth;

        [loop]
        for (uint refinementStep = 0; refinementStep < binaryRefinementSteps; ++refinementStep) {
            const float midpointT = (frontT + backT) * 0.5f;
            float midpointDepthDelta;
            float midpointSceneDepth;
            const bool midpointDepthValid = EvaluateScreenSpaceRayDeviceDepth(rayStart, rayDelta, midpointT, depthTextureSize, farPlaneDeviceDepth, useMinDepth, midpointDepthDelta, midpointSceneDepth);

            if (!midpointDepthValid || midpointDepthDelta < 0.0f) {
                frontT = midpointT;
            } else {
                backT = midpointT;
                backSceneDeviceDepth = midpointSceneDepth;
            }
        }

        // 앞뒤 순서 판정과 binary refinement 에는 device depth 만으로 충분하다.
        // Thickness 는 world-space meter 단위이므로 최종 후보만 linear view depth 로 변환한다.
        const float3 hitRayPosition = rayStart + rayDelta * backT;
        const float hitRayDepth = DeviceDepthToLinearDepth(hitRayPosition.z, ndcDepthScaleBias, projectionDepthParams, orthogonal);
        const float hitSceneDepth = DeviceDepthToLinearDepth(backSceneDeviceDepth, ndcDepthScaleBias, projectionDepthParams, orthogonal);
        const float hitDepthDelta = hitRayDepth - hitSceneDepth;

        if (hitDepthDelta >= 0.0f && hitDepthDelta <= thickness) {
            result.hitUV = hitRayPosition.xy;
            result.hitDeviceDepth = backSceneDeviceDepth;
            result.status = ScreenSpaceRayTraceStatusHit;
            return true;
        }
    }

    return false;
}

// rayStart / rayEnd 는 screen space 로 투영된 ray 의 양 끝점이다.
// xy = 정규화된 screen UV [0, 1), z = device depth [0, 1].
//
// View space ray 구간은 perspective divide 전에 near/camera plane 으로 clipping 되어야 한다.
// 이 함수는 이미 투영된 ray 를 순회한다.
ScreenSpaceRayTraceResult TraceScreenSpaceRayHiZ(float3 rayStart, float3 rayEnd, uint2 depthTextureSize, uint depthPyramidMipCount, uint maxIterations, uint binaryRefinementSteps,
    float thickness, float farPlaneDeviceDepth, bool useMinDepth, float2 ndcDepthScaleBias, float2 projectionDepthParams, bool orthogonal) {
    ScreenSpaceRayTraceResult result;
    result.hitUV = 0.0f;
    result.hitDeviceDepth = farPlaneDeviceDepth;
    result.status = ScreenSpaceRayTraceStatusMiss;
    result.iterationCount = 0;

    // Mip 0 pixel space 에서 순회한다.
    // Mip level m 의 Hi-Z cell 하나는 2^m x 2^m 개의 mip 0 pixel 을 포함한다.
    const float3 rayDelta = rayEnd - rayStart;
    // Mip 0 은 full-resolution depth texture 를 2x2 로 축소한 결과다.
    // 원본 크기의 절반을 사용하면 홀수 해상도에서도 원본 pixel pair 경계가 유지된다.
    const float2 mip0Scale = float2(depthTextureSize) * 0.5f;
    const float2 rayStartMip0 = rayStart.xy * mip0Scale;
    const float2 rayDeltaMip0 = rayDelta.xy * mip0Scale;
    const bool2 hasRayDeltaMip0 = abs(rayDeltaMip0) >= FLT_EPSILON;
    const float2 inverseRayDeltaMip0 = float2(
        hasRayDeltaMip0.x ? rcp(rayDeltaMip0.x) : 0.0f,
        hasRayDeltaMip0.y ? rcp(rayDeltaMip0.y) : 0.0f);
    
    const float maxScreenDelta = max(abs(rayDeltaMip0.x), abs(rayDeltaMip0.y));
    if (maxScreenDelta < FLT_EPSILON || depthPyramidMipCount == 0) {
        return result;
    }

    // Cell 경계를 지난 뒤 pixel 의 매우 작은 비율만큼 ray 를 전진시킨다.
    // 부동소수점 오차로 동일한 경계가 반복 판정되는 것을 방지한다.
    const float parameterEpsilon = 0.0001f * rcp(maxScreenDelta);
    const uint maxMipLevel = depthPyramidMipCount - 1;
    uint mipLevel = 0;
    float currentT = 0.0f;

    [loop]
    for (uint iteration = 0; iteration < maxIterations; ++iteration) {
        result.iterationCount = iteration + 1;

        const float3 currentRayPosition = rayStart + rayDelta * currentT;
        if (currentT >= 1.0f || !IsScreenSpaceRayInsideViewport(currentRayPosition.xy) || currentRayPosition.z < 0.0f || currentRayPosition.z > 1.0f) {
            return result;
        }

        const float2 currentMip0Position = rayStartMip0 + rayDeltaMip0 * currentT;
        const float cellSize = float(1u << mipLevel);
        const uint2 cellCoord = uint2(floor(currentMip0Position)) >> mipLevel;

        // Hi-Z 는 고정 DDA stride 대신 cell 크기에 따라 가변적으로 순회한다.
        // 현재 mip cell 에서 가장 먼저 만나는 X/Y 경계로 바로 이동한다.
        const float exitTX = FindScreenSpaceRayCellExit(rayStartMip0.x, rayDeltaMip0.x, inverseRayDeltaMip0.x, hasRayDeltaMip0.x, cellCoord.x, currentT, cellSize, parameterEpsilon);
        const float exitTY = FindScreenSpaceRayCellExit(rayStartMip0.y, rayDeltaMip0.y, inverseRayDeltaMip0.y, hasRayDeltaMip0.y, cellCoord.y, currentT, cellSize, parameterEpsilon);
        const float cellExitT = min(min(exitTX, exitTY), 1.0f);

        const float2 depthExtrema = inputDepthPyramidTexture.Load(int3(int2(cellCoord), int(mipLevel)));

        // Pyramid 에 저장된 device depth 수치 범위:
        // x = 최대 device depth, y = 최소 device depth.
        // Clear sample 만 있거나 padding 된 cell 은 뒤집힌 무효 범위(min > max)를 사용한다.
        const bool nonEmptyCell = depthExtrema.y <= depthExtrema.x;

        // Device depth 는 투영된 screen-space 구간을 따라 선형으로 변한다.
        // 물리적인 thickness 검사에서만 linear view depth 로 변환한다.
        const float rayDepthAtExit = rayStart.z + rayDelta.z * cellExitT;
        const float rayDepthMin = min(currentRayPosition.z, rayDepthAtExit);
        const float rayDepthMax = max(currentRayPosition.z, rayDepthAtExit);

        // Coarse depth 범위와 겹쳤다고 실제 hit 인 것은 아니다.
        // 현재 mip level 에서 이 cell 을 안전하게 건너뛸 수 없다는 의미일 뿐이다.
        // Miss    -> cell 경계를 넘어간 뒤 더 coarse 한 mip 을 검사한다.
        // Overlap -> ray 위치를 유지하고 더 fine 한 mip 으로 내려간다.
        const bool overlapsDepthRange = nonEmptyCell && rayDepthMax >= depthExtrema.y && rayDepthMin <= depthExtrema.x;

        // Mip 0 에서는 ray 와 scene depth 가 실제로 교차하는지 검증한다.
        // Hi-Z hierarchy 는 교차 가능성이 있는 영역만 찾았으므로,
        // 최종 hit 판정에는 full-resolution depth 검사가 필요하다.
        if (overlapsDepthRange && mipLevel > 0) {
            mipLevel--;
            continue;
        }

        if (overlapsDepthRange) {
            // Refinement 끝점을 현재 mip 0 cell 내부에 유지한다.
            // Cell 출구 경계를 정확히 sampling 하면 인접 cell 의 depth 를 읽을 수 있다.
            const float refinementExitT = cellExitT < 1.0f ? max(currentT, cellExitT - parameterEpsilon) : cellExitT;

            if (TryRefineScreenSpaceRayHit(rayStart, rayDelta, currentT, refinementExitT, depthTextureSize, binaryRefinementSteps,
                thickness, farPlaneDeviceDepth, useMinDepth, ndcDepthScaleBias, projectionDepthParams, orthogonal, result)) {
                return result;
            }
        }

        if (cellExitT >= 1.0f) {
            return result;
        }

        // 현재 cell 은 miss 로 확정되었다.
        // 다음 cell 로 넘어간 뒤 한 단계 더 coarse 한 mip 을 검사하여,
        // 이후 반복에서 더 넓은 빈 영역을 건너뛸 수 있게 한다.
        currentT = min(cellExitT + parameterEpsilon, 1.0f);
        mipLevel = min(mipLevel + 1, maxMipLevel);
    }

    result.status = ScreenSpaceRayTraceStatusMaxIterations;
    return result;
}

#endif // __SCREEN_SPACE_RAY_MARCHING_HLSLI__
