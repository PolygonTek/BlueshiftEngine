#ifndef __SHADER_PACKING_HLSLI__
#define __SHADER_PACKING_HLSLI__

// [-1, 1] 범위의 단위 벡터를 RGB8 UNORM 으로 remap 해 패킹한다.
inline uint PackUnitVector3ToRGB8UNorm(float3 value) {
    const uint3 packedValue = uint3(floor(saturate(value * 0.5 + 0.5) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u);
}

// RGB8 UNORM 값을 [-1, 1] 범위의 float3 로 복원한다. 정규화된 방향으로 사용할 때는 normalize 가 필요하다.
inline float3 UnpackRGB8UNormToUnitVector3(uint value) {
    return float3(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu) / 255.0 * 2.0 - 1.0;
}

// [-1, 1] 범위의 4성분 단위 벡터를 RGBA8 UNORM 으로 remap 해 패킹한다.
inline uint PackUnitVector4ToRGBA8UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value * 0.5 + 0.5) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u) | (packedValue.w << 24u);
}

// RGBA8 UNORM 값을 [-1, 1] 범위의 float4 로 복원한다. 정규화된 벡터로 사용할 때는 normalize 가 필요하다.
inline float4 UnpackRGBA8UNormToUnitVector4(uint value) {
    return float4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, value >> 24u) / 255.0 * 2.0 - 1.0;
}

// [0, 1] 범위로 clamp 한 float4 를 반올림해 RGBA8 UNORM 으로 양자화한다.
inline uint PackFloat4ToRGBA8UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u) | (packedValue.w << 24u);
}

// RGBA8 UNORM 값을 [0, 1] 범위의 float4 로 복원한다.
inline float4 UnpackRGBA8UNormToFloat4(uint value) {
    return float4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, value >> 24u) / 255.0;
}

// [0, 1] 범위로 clamp 한 float4 를 RGB 10-bit 와 alpha 2-bit 의 RGB10A2 UNORM 으로 양자화한다.
inline uint PackFloat4ToRGB10A2UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value) * float4(1023.0, 1023.0, 1023.0, 3.0) + 0.5));
    return packedValue.x | (packedValue.y << 10u) | (packedValue.z << 20u) | (packedValue.w << 30u);
}

// RGB10A2 UNORM 값을 [0, 1] 범위의 float4 로 복원한다. alpha 의 정밀도는 2-bit 이다.
inline float4 UnpackRGB10A2UNormToFloat4(uint value) {
    return float4(value & 0x3FFu, (value >> 10u) & 0x3FFu, (value >> 20u) & 0x3FFu, value >> 30u) / float4(1023.0, 1023.0, 1023.0, 3.0);
}

// float2 를 IEEE 754 half precision 두 개로 변환해 RG16F 로 패킹한다.
inline uint PackFloat2ToRG16F(float2 value) {
    return f32tof16(value.x) | (f32tof16(value.y) << 16u);
}

// RG16F 값을 IEEE 754 half precision 두 개로 해석해 float2 로 복원한다.
inline float2 UnpackRG16FToFloat2(uint value) {
    return float2(f16tof32(value & 0xFFFFu), f16tof32(value >> 16u));
}

// float3 를 IEEE 754 half precision 세 개로 변환해 RGB16F 로 패킹한다.
inline uint2 PackFloat3ToRGB16F(float3 value) {
    return uint2(PackFloat2ToRG16F(value.xy), f32tof16(value.z));
}

// RGB16F 값을 IEEE 754 half precision 세 개로 해석해 float3 로 복원한다.
inline float3 UnpackRGB16FToFloat3(uint2 value) {
    return float3(UnpackRG16FToFloat2(value.x), f16tof32(value.y & 0xFFFFu));
}

// float4 를 IEEE 754 half precision 네 개로 변환해 RGBA16F 로 패킹한다.
inline uint2 PackFloat4ToRGBA16F(float4 value) {
    return uint2(PackFloat2ToRG16F(value.xy), PackFloat2ToRG16F(value.zw));
}

// RGBA16F 값을 IEEE 754 half precision 네 개로 해석해 float4 로 복원한다.
inline float4 UnpackRGBA16FToFloat4(uint2 value) {
    return float4(UnpackRG16FToFloat2(value.x), UnpackRG16FToFloat2(value.y));
}

// uint2 각 성분의 하위 16-bit 를 RG16 UINT 형식으로 패킹한다.
inline uint PackUint2ToRG16Uint(uint2 value) {
    return (value.x & 0xFFFFu) | ((value.y & 0xFFFFu) << 16u);
}

// RG16 UINT 값을 두 개의 16-bit unsigned integer 로 복원한다.
inline uint2 UnpackRG16UintToUint2(uint value) {
    return uint2(value & 0xFFFFu, value >> 16u);
}

// [0, 1] 범위로 clamp 한 float2 를 반올림해 RG16 UNORM 으로 양자화한다.
inline uint PackFloat2ToRG16UNorm(float2 value) {
    const uint2 packedValue = uint2(floor(saturate(value) * 65535.0 + 0.5));
    return PackUint2ToRG16Uint(packedValue);
}

// RG16 UNORM 값을 [0, 1] 범위의 float2 로 복원한다.
inline float2 UnpackRG16UNormToFloat2(uint value) {
    return float2(value & 0xFFFFu, value >> 16u) / 65535.0;
}

// [0, 1] 범위로 clamp 한 float4 를 반올림해 RGBA16 UNORM 으로 양자화한다.
inline uint2 PackFloat4ToRGBA16UNorm(float4 value) {
    return uint2(PackFloat2ToRG16UNorm(value.xy), PackFloat2ToRG16UNorm(value.zw));
}

// RGBA16 UNORM 값을 [0, 1] 범위의 float4 로 복원한다.
inline float4 UnpackRGBA16UNormToFloat4(uint2 value) {
    return float4(UnpackRG16UNormToFloat2(value.x), UnpackRG16UNormToFloat2(value.y));
}

#endif // __SHADER_PACKING_HLSLI__
