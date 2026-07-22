#ifndef __CUBEMAP_HLSLI__
#define __CUBEMAP_HLSLI__

// top-left 가 (0, 0) 인 큐브맵 페이스 UV 를 큐브맵 샘플링 방향벡터로 변환한다.
// 큐브맵 샘플링에는 단위벡터가 필요하지 않으므로 리턴 방향은 normalize 하지 않는다.
float3 CubeMapFaceToDirection(uint faceIndex, float2 uv) {
    const float2 faceCoord = uv * 2.0f - 1.0f;

    switch (faceIndex) {
    case 0u: // PositiveX
        return float3(1.0f, -faceCoord.y, -faceCoord.x);
    case 1u: // NegativeX
        return float3(-1.0f, -faceCoord.y, faceCoord.x);
    case 2u: // PositiveY
        return float3(faceCoord.x, 1.0f, faceCoord.y);
    case 3u: // NegativeY
        return float3(faceCoord.x, -1.0f, -faceCoord.y);
    case 4u: // PositiveZ
        return float3(faceCoord.x, -faceCoord.y, 1.0f);
    case 5u: // NegativeZ
        return float3(-faceCoord.x, -faceCoord.y, -1.0f);
    default:
        return 0.0f;
    }
}

float CubeMapAreaElement(float x, float y) {
    return atan2(x * y, sqrt(x * x + y * y + 1.0f));
}

// texelCenterUV 를 중심으로 하는 큐브맵 텍셀이 차지하는 입체각을 반환한다.
float CubeMapTexelSolidAngle(float2 texelCenterUV, float invSize) {
    const float2 faceCoord = texelCenterUV * 2.0f - 1.0f;
    const float x0 = faceCoord.x - invSize;
    const float y0 = faceCoord.y - invSize;
    const float x1 = faceCoord.x + invSize;
    const float y1 = faceCoord.y + invSize;

    return CubeMapAreaElement(x0, y0) - CubeMapAreaElement(x0, y1) - CubeMapAreaElement(x1, y0) + CubeMapAreaElement(x1, y1);
}

#endif // __CUBEMAP_HLSLI__
