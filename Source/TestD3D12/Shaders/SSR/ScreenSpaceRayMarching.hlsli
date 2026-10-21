#ifndef __SCREEN_SPACE_RAY_MARCHING_HLSLI__
#define __SCREEN_SPACE_RAY_MARCHING_HLSLI__

struct ScreenSpaceRayTraceResult {
    float2 hitUV;
    float hitDepth;
    float hitDepthDelta;
    uint status;
    uint iterationCount;
};

// 현재 mip cell 에서 ray 진행 방향으로 가장 먼저 만나는 축 경계의 parameter t 를 구한다.
// 해당 축으로 이동하지 않는 경우에는 FLT_MAX 를 반환하여 다른 축의 경계가 선택되게 한다.
float FindCellExit(float rayStart, float inverseRayDir, int rayStep, int cellIndex, float cellSize) {
    if (rayStep == 0) {
        return FLT_MAX;
    }
    const float boundary = float(cellIndex + (rayStep > 0 ? 1 : 0)) * cellSize;
    return (boundary - rayStart) * inverseRayDir;
}

// 경계 위에서는 ray 가 진입하는 쪽의 child 를 선택한다. XY 를 다시 투영하여 floor 하면
// 거의 축 방향인 ray 의 작은 이동이 반올림되어 이미 통과한 경계의 반대쪽 child 를 고를 수 있다.
int SelectRayChild(float rayStart, float center, float centerT, int rayStep, float currentT) {
    if (rayStep == 0) {
        return rayStart >= center ? 1 : 0;
    }
    return (rayStep > 0 ? currentT >= centerT : currentT < centerT) ? 1 : 0;
}

// Mip 0 의 2x2 cell 을 내부 X/Y pixel 경계에서 나눈다. 직선 ray 는 최대 3개 pixel 을 순서대로 통과한다.
// Cell 양 끝만 검사하면 내부의 앞 -> 뒤 -> 앞 교차를 놓치므로, 각 pixel 구간에서 refinement 와 thickness 를 검사한다.
bool TryRefineRayHit(float3 rayStart, float3 rayDir, float entryT, float exitT, float2 pixelCrossingT, int2 pixelCoord, int2 rayStep, uint2 depthTextureSize,
    float thickness, float clearDepth, bool reverseZ, float2 projectionScaleBiasZ, bool orthogonal, inout ScreenSpaceRayTraceResult result) {
    const float2 pixelSplitT = clamp(float2(min(pixelCrossingT.x, pixelCrossingT.y), max(pixelCrossingT.x, pixelCrossingT.y)), entryT, exitT);
    float pixelEntryT = entryT;

    [loop]
    for (uint pixelIndex = 0u; pixelIndex < 3u; ++pixelIndex) {
        const float pixelExitT = pixelIndex == 0u ? pixelSplitT.x : (pixelIndex == 1u ? pixelSplitT.y : exitT);
        const float segmentEntryT = pixelEntryT;
        pixelEntryT = pixelExitT;
        // Ray 가 cell 중앙선을 지나지 않거나 corner 를 정확히 지날 때의 빈 구간은 건너뛴다.
        if (pixelExitT <= segmentEntryT) {
            continue;
        }

        // 구간의 pixel 은 경계 통과 방향으로 갱신한다. 짧은 구간의 중점을 다시 투영하면
        // float 반올림으로 인접 pixel 을 읽을 수 있다. 정확한 corner 교차는 두 축을 함께 갱신한다.
        const int2 sampleCoord = pixelCoord;
        pixelCoord += int2(pixelCrossingT.x == pixelExitT ? rayStep.x : 0, pixelCrossingT.y == pixelExitT ? rayStep.y : 0);
        // 홀수 해상도에서 마지막 2x2 cell 의 viewport 밖 구간도 제외한다.
        if (any(sampleCoord < 0) || any(sampleCoord >= int2(depthTextureSize))) {
            continue;
        }
        const float sceneDepth = inputSceneDepthTexture.Load(int3(sampleCoord, 0));
        // 원본 깊이를 보간 없이 읽으므로 clear 값만 배경으로 취급한다.
        if (sceneDepth == clearDepth) {
            continue;
        }

        const float entryDepth = rayStart.z + rayDir.z * segmentEntryT;
        const float exitDepth = rayStart.z + rayDir.z * pixelExitT;
        // Device depth 의 수치 방향을 정규화하여 음수는 surface 앞, 양수는 surface 뒤를 의미한다.
        const float entryDepthDelta = reverseZ ? sceneDepth - entryDepth : entryDepth - sceneDepth;
        const float exitDepthDelta = reverseZ ? sceneDepth - exitDepth : exitDepth - sceneDepth;

        float backT = segmentEntryT;
        if (entryDepthDelta < 0.0f) {
            if (exitDepthDelta < 0.0f) {
                continue;
            }
            // 한 pixel 안의 scene depth 는 일정하고 ray device depth 는 선형이다.
            // 반복 refinement 오차로 얇은 교차를 버리지 않도록 교차 parameter 를 직접 계산한다.
            const float fraction = saturate(-entryDepthDelta / (exitDepthDelta - entryDepthDelta));
            backT = lerp(segmentEntryT, pixelExitT, fraction);
        }

        // Pixel 경계에서 이미 surface 뒤로 진입했더라도 thickness 안에 있는 후보만 허용한다.
        // 앞뒤 순서 판정과 교차 parameter 계산에는 device depth 만으로 충분하다.
        // Thickness 는 world-space meter 단위이므로 최종 후보만 linear view depth 로 변환한다.
        const float3 hitRayPos = rayStart + rayDir * backT;
        if (hitRayPos.z < 0.0f || hitRayPos.z > 1.0f || any(hitRayPos.xy < 0.0f) || any(hitRayPos.xy >= 1.0f)) {
            continue;
        }
        const float hitRayDepth = ToLinearDepth(hitRayPos.z, projectionScaleBiasZ, orthogonal);
        const float hitSceneDepth = ToLinearDepth(sceneDepth, projectionScaleBiasZ, orthogonal);

        // 두께는 유한한 위치 사이에서만 비교한다.
        if (!isfinite(hitRayDepth) || !isfinite(hitSceneDepth)) {
            continue;
        }

        // A bracketed analytic crossing lies on the surface. Do not turn rounding at the
        // root into a negative thickness, including when the configured thickness is zero.
        const float hitDepthDelta = entryDepthDelta < 0.0f ? 0.0f : hitRayDepth - hitSceneDepth;

        if (hitDepthDelta >= 0.0f && hitDepthDelta <= thickness) {
            result.hitUV = hitRayPos.xy;
            result.hitDepth = sceneDepth;
            result.hitDepthDelta = hitDepthDelta;
            result.status = ScreenSpaceRayTraceStatusHit;
            return true;
        }
    }

    return false;
}

bool DoesRayOverlapDepthRange(float rayDepthMin, float rayDepthMax, float2 depthExtrema, float thickness, bool reverseZ, float2 projectionScaleBiasZ, bool orthogonal) {
    // SSRHiZ 의 x 는 최대 device depth, y 는 최소 device depth 이다.
    // Clear sample 만 있거나 padding 된 cell 은 뒤집힌 무효 범위(min > max)를 사용한다.
    if (depthExtrema.y > depthExtrema.x) {
        return false;
    }

    [branch]
    if (rayDepthMax >= depthExtrema.y && rayDepthMin <= depthExtrema.x) {
        return true;
    }

    // 최종 pixel 검사는 surface 뒤쪽의 thickness 도 허용한다. Coarse 검사에서 이 후보를 먼저 버리지 않는다.
    // 표면 앞쪽은 확장하지 않고, 기존 범위보다 뒤에 있는 ray 만 linear depth 로 변환한다.
    const float closestRayDepth = reverseZ ? rayDepthMax : rayDepthMin;
    const float farthestSceneDepth = reverseZ ? depthExtrema.y : depthExtrema.x;
    const bool rayBehindScene = reverseZ ? closestRayDepth < farthestSceneDepth : closestRayDepth > farthestSceneDepth;
    [branch]
    if (thickness <= 0.0f || !rayBehindScene) {
        return false;
    }

    const float closestRayLinearDepth = ToLinearDepth(closestRayDepth, projectionScaleBiasZ, orthogonal);
    const float farthestSceneLinearDepth = ToLinearDepth(farthestSceneDepth, projectionScaleBiasZ, orthogonal);
    return isfinite(closestRayLinearDepth) && isfinite(farthestSceneLinearDepth) && closestRayLinearDepth - farthestSceneLinearDepth <= thickness;
}

// rayStart 는 screen UV/device depth 이고 rayDir 은 같은 공간의 진행 방향이다.
// rayStart + rayDir * t (t >= 0) 를 따라 추적하며, 방향은 단위 벡터일 필요가 없다.
// 출발점은 카메라 전방의 유효한 표면이며 UV 는 [0, 1), device depth 는 [0, 1] 이다.
ScreenSpaceRayTraceResult TraceScreenSpaceRayHiZ(float3 rayStart, float3 rayDir, uint2 depthTextureSize, uint ssrHiZMipCount, uint maxIterations, float thickness, float clearDepth, bool reverseZ, float2 projectionScaleBiasZ, bool orthogonal) {
    ScreenSpaceRayTraceResult result;
    result.hitUV = 0.0f;
    result.hitDepth = clearDepth;
    result.hitDepthDelta = FLT_MAX;
    result.status = ScreenSpaceRayTraceStatusMiss;
    result.iterationCount = 0;

    // Mip 0 pixel space 에서 순회한다.
    // Mip level m 의 Hi-Z cell 하나는 2^m x 2^m 개의 mip 0 pixel 을 포함한다.
    // Mip 0 은 full-resolution depth texture 를 2x2 로 축소한 결과다.
    // 원본 크기의 절반을 사용하면 홀수 해상도에서도 원본 pixel pair 경계가 유지된다.
    const float2 mip0Scale = float2(depthTextureSize) * 0.5f;
    const int2 validMip0Size = int2((depthTextureSize + 1u) >> 1u);
    const float2 rayStartMip0 = rayStart.xy * mip0Scale;
    const float2 rayDirMip0 = rayDir.xy * mip0Scale;
    // 방향의 크기는 임의이므로 절대 epsilon 으로 작은 성분을 제거하지 않는다.
    const bool2 hasRayDirMip0 = rayDirMip0 != 0.0f;
    const float2 inverseRayDirMip0 = float2(hasRayDirMip0.x ? rcp(rayDirMip0.x) : 0.0f, hasRayDirMip0.y ? rcp(rayDirMip0.y) : 0.0f);
    const int2 rayStep = int2(hasRayDirMip0.x ? (rayDirMip0.x > 0.0f ? 1 : -1) : 0, hasRayDirMip0.y ? (rayDirMip0.y > 0.0f ? 1 : -1) : 0);

    if (!any(hasRayDirMip0) || ssrHiZMipCount == 0 || !all(isfinite(rayStart)) || !all(isfinite(rayDir)) ||
        any(rayStart.xy < 0.0f) || any(rayStart.xy >= 1.0f) || rayStart.z < 0.0f || rayStart.z > 1.0f) {
        return result;
    }

    // 마지막 cell 에서도 유효한 깊이 구간을 검사하도록, 진행 방향의 depth 경계 parameter 만 구한다.
    // 무한 far 투영도 해당 depth 경계까지 추적하며, 깊이가 일정한 ray 는 XY 경계로 종료한다.
    const float depthExitT = rayDir.z != 0.0f ? ((rayDir.z > 0.0f ? 1.0f : 0.0f) - rayStart.z) / rayDir.z : FLT_MAX;

    // 정확한 경계에서 음의 방향으로 출발할 때만
    // 진입할 cell 의 주소를 선택하며, 이후에는 통과한 축의 정수 cell 좌표를 갱신한다.
    const float2 startCell = floor(rayStartMip0);
    int2 cellCoord = int2(startCell) - int2(rayStep.x < 0 && rayStartMip0.x == startCell.x ? 1 : 0, rayStep.y < 0 && rayStartMip0.y == startCell.y ? 1 : 0);

    // 출발 표면과의 즉시 self-intersection 을 피하도록 출발 mip 0 cell (원본 2x2 pixel) 은 검사하지 않는다.
    // Ray 는 원래 직선 위의 출구 경계로 전진하고, 정수 cell 좌표로 다음 cell 을 선택하므로 epsilon 은 필요 없다.
    const float startExitTX = FindCellExit(rayStartMip0.x, inverseRayDirMip0.x, rayStep.x, cellCoord.x, 1.0f);
    const float startExitTY = FindCellExit(rayStartMip0.y, inverseRayDirMip0.y, rayStep.y, cellCoord.y, 1.0f);
    float currentT = min(startExitTX, startExitTY);
    if (currentT >= depthExitT || !isfinite(currentT)) {
        return result;
    }
    cellCoord += int2(startExitTX <= startExitTY ? rayStep.x : 0, startExitTY <= startExitTX ? rayStep.y : 0);

    const uint maxMipLevel = ssrHiZMipCount - 1;
    uint mipLevel = 0;

    [loop]
    for (uint iteration = 0; iteration < maxIterations; ++iteration) {
        result.iterationCount = iteration + 1;

        const float3 currentRayPosition = rayStart + rayDir * currentT;
        // 화면 끝의 UV 가 안쪽으로 반올림되어도 범위를 벗어난 cell 은 읽지 않는다.
        if (currentT >= depthExitT || any(cellCoord < 0) || any((cellCoord << mipLevel) >= validMip0Size) ||
            any(currentRayPosition.xy < 0.0f) || any(currentRayPosition.xy >= 1.0f) || currentRayPosition.z < 0.0f || currentRayPosition.z > 1.0f) {
            return result;
        }

        const float cellSize = float(1u << mipLevel);

        // Hi-Z 는 고정 DDA stride 대신 cell 크기에 따라 가변적으로 순회한다.
        // 현재 mip cell 에서 가장 먼저 만나는 X/Y 경계로 바로 이동한다.
        const float exitTX = FindCellExit(rayStartMip0.x, inverseRayDirMip0.x, rayStep.x, cellCoord.x, cellSize);
        const float exitTY = FindCellExit(rayStartMip0.y, inverseRayDirMip0.y, rayStep.y, cellCoord.y, cellSize);
        // Depth 경계가 cell 내부에 있으면 그 앞의 교차까지 검사한 뒤 종료한다.
        const float cellExitT = max(currentT, min(min(exitTX, exitTY), depthExitT));

        const float2 depthExtrema = inputSSRHiZTexture.Load(int3(int2(cellCoord), int(mipLevel)));

        // Device depth 는 투영된 screen-space 구간을 따라 선형으로 변한다.
        // 물리적인 thickness 검사에서만 linear view depth 로 변환한다.
        const float rayDepthAtExit = rayStart.z + rayDir.z * cellExitT;
        const float rayDepthMin = min(currentRayPosition.z, rayDepthAtExit);
        const float rayDepthMax = max(currentRayPosition.z, rayDepthAtExit);

        // Coarse depth 범위와 겹쳤다고 실제 hit 인 것은 아니다.
        // 현재 mip level 에서 이 cell 을 안전하게 건너뛸 수 없다는 의미일 뿐이다.
        // Miss    -> cell 경계를 넘어간 뒤 더 coarse 한 mip 을 검사한다.
        // Overlap -> ray 위치를 유지하고 더 fine 한 mip 으로 내려간다.
        const bool overlapsDepthRange = DoesRayOverlapDepthRange(rayDepthMin, rayDepthMax, depthExtrema, thickness, reverseZ, projectionScaleBiasZ, orthogonal);

        // Mip 0 에서는 ray 와 scene depth 의 실제 교차 또는 thickness 안의 진입인지 검증한다.
        // Hi-Z hierarchy 는 교차 가능성이 있는 영역만 찾았으므로,
        // 최종 hit 판정에는 full-resolution depth 검사가 필요하다.
        if (overlapsDepthRange) {
            // Parent 의 중앙 경계와 같은 parameter 기준으로 child 를 선택한다.
            // Mip 0 의 child 는 원본 2x2 pixel 이다. 역방향 벡터를 재사용해 나눗셈을 피한다.
            const float2 cellCenter = (float2(cellCoord) + 0.5f) * cellSize;
            const float2 centerT = float2(hasRayDirMip0.x ? (cellCenter.x - rayStartMip0.x) * inverseRayDirMip0.x : FLT_MAX, hasRayDirMip0.y ? (cellCenter.y - rayStartMip0.y) * inverseRayDirMip0.y : FLT_MAX);
            const int2 childCoord = cellCoord * 2 + int2(
                SelectRayChild(rayStartMip0.x, cellCenter.x, centerT.x, rayStep.x, currentT),
                SelectRayChild(rayStartMip0.y, cellCenter.y, centerT.y, rayStep.y, currentT));
            if (mipLevel > 0) {
                cellCoord = childCoord;
                mipLevel--;
                continue;
            }

            // Pixel 주소를 별도로 유지하므로 출구 앞의 epsilon 구간도 잘라내지 않고 검사한다.
            if (TryRefineRayHit(rayStart, rayDir, currentT, cellExitT, centerT, childCoord, rayStep, depthTextureSize,
                thickness, clearDepth, reverseZ, projectionScaleBiasZ, orthogonal, result)) {
                return result;
            }
        }

        if (cellExitT >= depthExitT) {
            return result;
        }

        // 현재 cell 은 miss 로 확정되었다. 통과한 축만 이웃 cell 로 옮기며,
        // 정확한 corner 는 두 축을 함께 옮긴다. 가까운 두 교차를 epsilon 으로 합치지는 않는다.
        // Ray 는 원래 직선 위의 경계에 유지하고, 다음 parent 에서 더 넓은 빈 영역을 건너뛴다.
        cellCoord += int2(exitTX <= exitTY ? rayStep.x : 0, exitTY <= exitTX ? rayStep.y : 0);
        currentT = cellExitT;
        if (mipLevel < maxMipLevel) {
            cellCoord >>= 1;
            mipLevel++;
        }
    }

    result.status = ScreenSpaceRayTraceStatusMaxIterations;
    return result;
}

#endif // __SCREEN_SPACE_RAY_MARCHING_HLSLI__
