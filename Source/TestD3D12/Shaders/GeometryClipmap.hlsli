#ifndef __GEOMETRY_CLIPMAP_HLSLI__
#define __GEOMETRY_CLIPMAP_HLSLI__

#include "GeometryClipmapInterop.h"

struct GeometryClipmapVertex {
    float2 position;
    float2 parentPosition;
    float morph;
};

GeometryClipmapVertex GetGeometryClipmapVertex(uint vertexID, GeometryClipmapPatch patch, float2 cameraPosition) {
    const uint2 localCoord = uint2(vertexID % patch.vertexResolution, vertexID / patch.vertexResolution);
    const int2 gridCoord = patch.gridOrigin + int2(localCoord);
    GeometryClipmapVertex result;
    result.position = float2(gridCoord) * patch.gridSpacing;

    // 전역 격자 위상에 따라 홀수 정점을 parent 정점으로 collapse 한다. 음수도 동일하다.
    // morph == 1 에서 각 2x2 cell 은 parent 의 두 삼각형과 일치하며 나머지는 퇴화한다.
    // 외곽 경계도 이 연산을 사용하므로 별도의 skirt/T-junction 이 없다.
    const int2 parentGridCoord = gridCoord & int2(-2, -2);
    result.parentPosition = float2(parentGridCoord) * patch.gridSpacing;
    // 같은 parent 로 합쳐지는 정점은 mip/XYZ 혼합 가중치도 같아야 한다.
    // fine 위치로 가중치를 계산하면 이미 겹친 정점이 서로 다른 변위를 가질 수 있다.
    const float2 distanceXY = abs(result.parentPosition - cameraPosition);
    const float alpha = saturate((max(distanceXY.x, distanceXY.y) - patch.morphStart) * patch.morphScale);
    result.morph = alpha * alpha * (3.0f - 2.0f * alpha);
    // 거리 계산의 float 오차와 관계없이 경계는 완전히 stitch 한다. 같은 parent 로
    // 합쳐지는 바로 안쪽 정점도 포함해야 변위/mip 가 달라지지 않는다.
    const int2 parentLocalCoord = parentGridCoord - patch.gridOrigin;
    const int width = int((patch.packedBounds >> 8u) & 0xffu);
    const int height = int((patch.packedBounds >> 16u) & 0xffu);
    const bool boundary = ((patch.packedBounds & 1u) != 0u && parentLocalCoord.x == 0) ||
        ((patch.packedBounds & 2u) != 0u && parentLocalCoord.x == width) ||
        ((patch.packedBounds & 4u) != 0u && parentLocalCoord.y == 0) ||
        ((patch.packedBounds & 8u) != 0u && parentLocalCoord.y == height);
    result.morph = boundary ? 1.0f : result.morph;
    return result;
}

// 소비자는 fine/parent 좌표에서 각각 변위까지 평가한 뒤 최종 XYZ 를 보간해야 한다.
// parent 를 현재 morph 좌표에서 다시 샘플링하면 완전 전환 시 coarse 메시와 달라진다.
float3 MorphGeometryClipmapPosition(float3 position, float3 parentPosition, float morph) {
    return lerp(position, parentPosition, morph);
}

#endif
