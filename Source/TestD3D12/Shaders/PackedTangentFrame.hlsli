#ifndef __PACKED_TANGENT_FRAME_HLSLI__
#define __PACKED_TANGENT_FRAME_HLSLI__

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

// normal 에 수직인 기준 tangent 벡터를 생성한다.
inline float3 BuildPackedTangentFrameBaseTangent(float3 normal) {
    const float3 baseTangent = abs(normal.x) > abs(normal.z) ? float3(-normal.y, normal.x, 0.0) : float3(0.0, -normal.z, normal.y);
    return normalize(baseTangent);
}

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
inline float EncodePackedTangentFrameDiamond(float2 direction) {
    const float l1Norm = abs(direction.x) + abs(direction.y);
    if (l1Norm <= 1e-20) {
        return 0.0;
    }

    const float diamondX = direction.x / l1Norm;
    const float ySign = direction.y >= 0.0 ? 1.0 : -1.0;
    const float diamond = -ySign * 0.25 * diamondX + 0.5 + ySign * 0.25;

    // Rotate the periodic coordinate so zero still represents baseTangent.
    return frac(diamond + 0.5);
}

inline float2 DecodePackedTangentFrameDiamond(float encoded) {
    const float diamond = frac(encoded + 0.5);
    const float diamondSign = diamond >= 0.5 ? 1.0 : -1.0;
    const float x = -diamondSign * 4.0 * diamond + 1.0 + diamondSign * 2.0;
    const float y = diamondSign * (1.0 - abs(x));
    return normalize(float2(x, y));
}
#endif

// normal 과 tangent 프레임을 11/11/9/1 비트 형식으로 패킹한다.
inline uint PackTangentFrame(float3 normal, float3 tangent, float bitangentSign) {
    static const uint normalMask = 0x7FFu;
    static const uint tangentMask = 0x1FFu;

    const float2 encodedNormal = EncodeUnitVectorOctahedral(normal);
    const uint2 quantizedNormal = (uint2)round(saturate(encodedNormal * 0.5 + 0.5) * normalMask);
    const float2 decodedNormalOct = float2(quantizedNormal) * (2.0 / normalMask) - 1.0;
    const float3 decodedNormal = DecodeUnitVectorOctahedral(decodedNormalOct);
    const float3 baseTangent = BuildPackedTangentFrameBaseTangent(decodedNormal);
    const float3 baseBitangent = cross(decodedNormal, baseTangent);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    const float2 tangentDirection = float2(dot(tangent, baseTangent), dot(tangent, baseBitangent));
    const uint quantizedTangent = (uint)round(EncodePackedTangentFrameDiamond(tangentDirection) * 512.0) & tangentMask;
#else
    static const float twoPi = 6.28318530717958647692;
    const float tangentAngle = atan2(dot(tangent, baseBitangent), dot(tangent, baseTangent));
    const uint quantizedTangent = (uint)round(frac(tangentAngle / twoPi + 1.0) * 512.0) & tangentMask;
#endif
    const uint signBit = bitangentSign < 0.0 ? 1u : 0u;
    return quantizedNormal.x | (quantizedNormal.y << 11u) | (quantizedTangent << 22u) | (signBit << 31u);
}

// 패킹된 tangent 프레임에서 normal 을 복원한다.
inline float3 UnpackTangentFrameNormal(uint packedTangentFrame) {
    static const uint normalMask = 0x7FFu;
    const uint2 quantizedNormal = uint2(packedTangentFrame & normalMask, (packedTangentFrame >> 11u) & normalMask);
    const float2 encodedNormal = float2(quantizedNormal) * (2.0 / normalMask) - 1.0;
    return DecodeUnitVectorOctahedral(encodedNormal);
}

// 복원된 normal 을 사용해 패킹된 tangent 를 복원한다.
inline float3 UnpackTangentFrameTangent(uint packedTangentFrame, float3 normal) {
    static const uint tangentMask = 0x1FFu;

    const float3 baseTangent = BuildPackedTangentFrameBaseTangent(normal);
    const float3 baseBitangent = cross(normal, baseTangent);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    const float encodedTangent = float((packedTangentFrame >> 22u) & tangentMask) / 512.0;
    const float2 tangentDirection = DecodePackedTangentFrameDiamond(encodedTangent);
    return baseTangent * tangentDirection.x + baseBitangent * tangentDirection.y;
#else
    const float tangentAngle = float((packedTangentFrame >> 22u) & tangentMask) * (6.28318530717958647692 / 512.0);
    float tangentSin;
    float tangentCos;
    sincos(tangentAngle, tangentSin, tangentCos);
    return baseTangent * tangentCos + baseBitangent * tangentSin;
#endif
}

// 패킹된 tangent 프레임에서 tangent 를 복원한다.
inline float3 UnpackTangentFrameTangent(uint packedTangentFrame) {
    return UnpackTangentFrameTangent(packedTangentFrame, UnpackTangentFrameNormal(packedTangentFrame));
}

// 패킹된 tangent 프레임 전체를 normal, tangent 와 bitangent 부호로 복원한다.
inline void UnpackTangentFrame(uint packedTangentFrame, out float3 normal, out float3 tangent, out float bitangentSign) {
    normal = UnpackTangentFrameNormal(packedTangentFrame);
    tangent = UnpackTangentFrameTangent(packedTangentFrame, normal);
    bitangentSign = (packedTangentFrame & 0x80000000u) != 0u ? -1.0 : 1.0;
}

#endif // __PACKED_TANGENT_FRAME_HLSLI__
