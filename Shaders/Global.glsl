#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#define FLT_SMALLEST        1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN             1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX             3.402823466e+38 // Maximum representable floating-point number
#define HALF_MAX            65504.0 // (2 - 2^-10) * 2^15
#define HALF_MAX_MINUS1     65472.0 // (2 - 2^-9) * 2^15
#define HALF_PI             1.5707963267948966192313216916398
#define PI                  3.1415926535897932384626433832795
#define TWO_PI              6.283185307179586476925286766559
#define FOUR_PI             12.566370614359172953850573533118
#define INV_HALF_PI         0.63661977236758134307553505349004
#define INV_PI              0.31830988618379067153776752674503
#define INV_TWO_PI          0.15915494309189533576888376337251
#define INV_FOUR_PI         0.07957747154594766788444188168626

float pow4(float f) {
    float f2 = f * f;
    return f2 * f2;
}

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
}

float log10(float x) {
    // 1.0 / log(10) = 0.4342944819
    return 0.4342944819 * log(x);
}

vec2 encodeUnitVectorOctahedral(vec3 unitVector) {
    unitVector /= abs(unitVector.x) + abs(unitVector.y) + abs(unitVector.z);
    if (unitVector.z < 0.0) {
        vec2 signs = vec2(unitVector.x >= 0.0 ? 1.0 : -1.0, unitVector.y >= 0.0 ? 1.0 : -1.0);
        unitVector.xy = (1.0 - abs(unitVector.yx)) * signs;
    }
    return unitVector.xy;
}

vec3 decodeUnitVectorOctahedral(vec2 encoded) {
    vec3 unitVector = vec3(encoded, 1.0 - abs(encoded.x) - abs(encoded.y));
    if (unitVector.z < 0.0) {
        vec2 signs = vec2(unitVector.x >= 0.0 ? 1.0 : -1.0, unitVector.y >= 0.0 ? 1.0 : -1.0);
        unitVector.xy = (1.0 - abs(unitVector.yx)) * signs;
    }
    return normalize(unitVector);
}

vec3 buildPackedTangentFrameBaseTangent(vec3 normal) {
    vec3 baseTangent = abs(normal.x) > abs(normal.z) ? vec3(-normal.y, normal.x, 0.0) : vec3(0.0, -normal.z, normal.y);
    return normalize(baseTangent);
}

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
float encodePackedTangentFrameDiamond(vec2 direction) {
    float l1Norm = abs(direction.x) + abs(direction.y);
    if (l1Norm <= 1e-20) {
        return 0.0;
    }

    float diamondX = direction.x / l1Norm;
    float ySign = direction.y >= 0.0 ? 1.0 : -1.0;
    float diamond = -ySign * 0.25 * diamondX + 0.5 + ySign * 0.25;

    // Rotate the periodic coordinate so zero still represents baseTangent.
    return fract(diamond + 0.5);
}

vec2 decodePackedTangentFrameDiamond(float encoded) {
    float diamond = fract(encoded + 0.5);
    float diamondSign = diamond >= 0.5 ? 1.0 : -1.0;
    float x = -diamondSign * 4.0 * diamond + 1.0 + diamondSign * 2.0;
    float y = diamondSign * (1.0 - abs(x));
    return normalize(vec2(x, y));
}
#endif

uint packTangentFrame(vec3 normal, vec3 tangent, float bitangentSign) {
    const uint normalMask = 0x7FFu;
    const uint tangentMask = 0x1FFu;

    vec2 encodedNormal = encodeUnitVectorOctahedral(normal);
    uvec2 quantizedNormal = uvec2(round(clamp(encodedNormal * 0.5 + 0.5, 0.0, 1.0) * float(normalMask)));
    vec2 decodedNormalOct = vec2(quantizedNormal) * (2.0 / float(normalMask)) - 1.0;
    vec3 decodedNormal = decodeUnitVectorOctahedral(decodedNormalOct);
    vec3 baseTangent = buildPackedTangentFrameBaseTangent(decodedNormal);
    vec3 baseBitangent = cross(decodedNormal, baseTangent);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    vec2 tangentDirection = vec2(dot(tangent, baseTangent), dot(tangent, baseBitangent));
    uint quantizedTangent = uint(round(encodePackedTangentFrameDiamond(tangentDirection) * 512.0)) & tangentMask;
#else
    const float twoPi = 6.28318530717958647692;
    float tangentAngle = atan(dot(tangent, baseBitangent), dot(tangent, baseTangent));
    uint quantizedTangent = uint(round(fract(tangentAngle / twoPi + 1.0) * 512.0)) & tangentMask;
#endif
    uint signBit = bitangentSign < 0.0 ? 1u : 0u;
    return quantizedNormal.x | (quantizedNormal.y << 11u) | (quantizedTangent << 22u) | (signBit << 31u);
}

vec3 unpackTangentFrameNormal(uint packedTangentFrame) {
    const uint normalMask = 0x7FFu;
    uvec2 quantizedNormal = uvec2(packedTangentFrame & normalMask, (packedTangentFrame >> 11u) & normalMask);
    vec2 encodedNormal = vec2(quantizedNormal) * (2.0 / float(normalMask)) - 1.0;
    return decodeUnitVectorOctahedral(encodedNormal);
}

vec3 unpackTangentFrameTangent(uint packedTangentFrame, vec3 normal) {
    const uint tangentMask = 0x1FFu;

    vec3 baseTangent = buildPackedTangentFrameBaseTangent(normal);
    vec3 baseBitangent = cross(normal, baseTangent);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    float encodedTangent = float((packedTangentFrame >> 22u) & tangentMask) / 512.0;
    vec2 tangentDirection = decodePackedTangentFrameDiamond(encodedTangent);
    return baseTangent * tangentDirection.x + baseBitangent * tangentDirection.y;
#else
    float tangentAngle = float((packedTangentFrame >> 22u) & tangentMask) * (6.28318530717958647692 / 512.0);
    return baseTangent * cos(tangentAngle) + baseBitangent * sin(tangentAngle);
#endif
}

void unpackTangentFrame(uint packedTangentFrame, out vec3 normal, out vec4 tangent) {
    normal = unpackTangentFrameNormal(packedTangentFrame);
    tangent.xyz = unpackTangentFrameTangent(packedTangentFrame, normal);
    tangent.w = (packedTangentFrame & 0x80000000u) != 0u ? -1.0 : 1.0;
}

vec3 unpackTangentFrameTangent(uint packedTangentFrame) {
    vec3 normal = unpackTangentFrameNormal(packedTangentFrame);
    return unpackTangentFrameTangent(packedTangentFrame, normal);
}

vec4 unpackTangentFrameTangent4(uint packedTangentFrame) {
    vec3 normal;
    vec4 tangent;
    unpackTangentFrame(packedTangentFrame, normal, tangent);
    return tangent;
}

#endif
