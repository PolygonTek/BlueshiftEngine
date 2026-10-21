#ifndef __STBN_HLSLI__
#define __STBN_HLSLI__

// NVIDIA STBN Vec2: 128x128 픽셀, 64개 시간 슬라이스의 RG8_UNORM [0, 1]^2 샘플.
float2 ReadSTBNVec2(Texture2DArray<float4> stbnTexture, uint2 pixelCoord, uint frameIndex) {
    const int4 coord = int4(pixelCoord & 127u, frameIndex & 63u, 0);
    return stbnTexture.Load(coord).rg;
}

// NVIDIA STBNUnitVec2: 128x128 픽셀, 64개 시간 슬라이스의 RG8_UNORM 방향 샘플.
// 필터링 없이 읽고 8비트 양자화로 생긴 길이 오차를 보정해 (cos, sin) 을 복원한다.
float2 ReadSTBNUnitVec2(Texture2DArray<float4> stbnTexture, uint2 pixelCoord, uint frameIndex) {
    const int4 coord = int4(pixelCoord & 127u, frameIndex & 63u, 0);
    return normalize(stbnTexture.Load(coord).rg * 2.0f - 1.0f);
}

#endif // __STBN_HLSLI__
