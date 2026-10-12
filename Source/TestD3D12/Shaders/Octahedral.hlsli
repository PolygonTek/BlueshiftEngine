#ifndef __OCTAHEDRAL_HLSLI__
#define __OCTAHEDRAL_HLSLI__

// 단위 벡터를 octahedral 좌표로 인코딩한다.
inline float2 EncodeUnitVectorOctahedral(float3 unitVector) {
    unitVector /= abs(unitVector.x) + abs(unitVector.y) + abs(unitVector.z);
    if (unitVector.z < 0.0) {
        const float2 signs = float2(unitVector.x >= 0.0 ? 1.0 : -1.0, unitVector.y >= 0.0 ? 1.0 : -1.0);
        unitVector.xy = (1.0 - abs(unitVector.yx)) * signs;
    }
    return unitVector.xy;
}

// octahedral 좌표를 단위 벡터로 디코딩한다.
inline float3 DecodeUnitVectorOctahedral(float2 encoded) {
    float3 unitVector = float3(encoded, 1.0 - abs(encoded.x) - abs(encoded.y));
    if (unitVector.z < 0.0) {
        const float2 signs = float2(unitVector.x >= 0.0 ? 1.0 : -1.0, unitVector.y >= 0.0 ? 1.0 : -1.0);
        unitVector.xy = (1.0 - abs(unitVector.yx)) * signs;
    }
    return normalize(unitVector);
}

#endif // __OCTAHEDRAL_HLSLI__
