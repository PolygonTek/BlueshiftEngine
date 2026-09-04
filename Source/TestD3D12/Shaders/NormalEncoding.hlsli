#ifndef __NORMAL_ENCODING_HLSLI__
#define __NORMAL_ENCODING_HLSLI__

float2 EncodeOctahedralNormal(float3 normal) {
    normal /= abs(normal.x) + abs(normal.y) + abs(normal.z);

    if (normal.z < 0.0f) {
        const float2 signNotZero = float2(normal.x >= 0.0f ? 1.0f : -1.0f, normal.y >= 0.0f ? 1.0f : -1.0f);
        normal.xy = (1.0f - abs(normal.yx)) * signNotZero;
    }
    return normal.xy;
}

float3 DecodeOctahedralNormal(float2 encodedNormal) {
    float3 normal = float3(encodedNormal, 1.0f - abs(encodedNormal.x) - abs(encodedNormal.y));

    if (normal.z < 0.0f) {
        const float2 signNotZero = float2(normal.x >= 0.0f ? 1.0f : -1.0f, normal.y >= 0.0f ? 1.0f : -1.0f);
        normal.xy = (1.0f - abs(normal.yx)) * signNotZero;
    }
    return normalize(normal);
}

#endif // __NORMAL_ENCODING_HLSLI__
