// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Packed Tangent Frame

    Packs an orthonormal tangent frame into a 32-bit value. The normal is
    octahedrally encoded into two 11-bit UNorm components. The tangent direction
    around the normal is stored in 9 bits using diamond encoding when
    PACKED_TANGENT_FRAME_DIAMOND_ENCODING is defined, or an angle otherwise.
    The final bit stores the tangent-frame handedness.

     31                   22 21           11 10            0
    +-----------------------+---------------+---------------+
    | S 1bit | Tangent 9bit |  Oct Y 11bit  |  Oct X 11bit  |
    +-----------------------+---------------+---------------+

-------------------------------------------------------------------------------
*/

// Remove this define to use the angle-based tangent encoding instead.
#define PACKED_TANGENT_FRAME_DIAMOND_ENCODING

class PackedTangentFrame {
public:
                        /// Packs a normalized normal with the default tangent direction and positive bitangent sign.
    static uint32_t     PackNormal(const Vec3 &normal);

                        /// Packs normalized normal and tangent vectors with the bitangent sign.
    static uint32_t     Pack(const Vec3 &normal, const Vec3 &tangent, float bitangentSign);

                        /// Unpacks normalized normal and tangent vectors with the bitangent sign.
    static void         Unpack(uint32_t packed, Vec3 &normal, Vec3 &tangent, float &bitangentSign);

    static Vec3         UnpackNormal(uint32_t packed);
    static Vec3         UnpackTangent(uint32_t packed);
    static float        UnpackBiTangentSign(uint32_t packed);
    static uint32_t     ReplaceBiTangentSign(uint32_t packed, float bitangentSign);

private:
    static Vec3         BuildBaseTangent(const Vec3 &normal);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    static float        EncodeDiamond(const Vec2 &direction);
    static Vec2         DecodeDiamond(float encoded);
#endif
    static Vec3         ReconstructTangent(uint32_t packed, const Vec3 &normal);
    static uint32_t     QuantizeUNorm(float value, uint32_t maxValue);

    static constexpr uint32_t NormalBits = 11;
    static constexpr uint32_t TangentBits = 9;
    static constexpr uint32_t NormalMask = (1u << NormalBits) - 1u;
    static constexpr uint32_t TangentMask = (1u << TangentBits) - 1u;
    static constexpr uint32_t NormalYShift = NormalBits;
    static constexpr uint32_t TangentShift = NormalBits * 2;
    static constexpr uint32_t BiTangentSignShift = TangentShift + TangentBits;
};

BE_INLINE uint32_t PackedTangentFrame::PackNormal(const Vec3 &normal) {
    const Vec2 encodedNormal = Octahedral::Encode(normal) * 0.5f + Vec2(0.5f);
    const uint32_t normalX = QuantizeUNorm(encodedNormal.x, NormalMask);
    const uint32_t normalY = QuantizeUNorm(encodedNormal.y, NormalMask);

    // A zero tangent field reconstructs the deterministic base tangent, and the
    // cleared sign bit represents positive handedness for either encoding.
    return normalX | (normalY << NormalYShift);
}

BE_INLINE uint32_t PackedTangentFrame::Pack(const Vec3 &normal, const Vec3 &tangent, float bitangentSign) {
    const uint32_t packedNormal = PackNormal(normal);

    // Build the tangent basis from the quantized normal so packing and unpacking
    // cannot choose different basis branches near their boundary.
    const Vec3 quantizedNormal = UnpackNormal(packedNormal);
    const Vec3 baseTangent = BuildBaseTangent(quantizedNormal);
    const Vec3 baseBiTangent = Vec3::FromCross(quantizedNormal, baseTangent);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    const Vec2 tangentDirection(tangent.Dot(baseTangent), tangent.Dot(baseBiTangent));
    constexpr float tangentLevels = static_cast<float>(1u << TangentBits);
    const uint32_t quantizedTangent = static_cast<uint32_t>(EncodeDiamond(tangentDirection) * tangentLevels + 0.5f) & TangentMask;
#else
    float angle = Math::ATan2(tangent.Dot(baseBiTangent), tangent.Dot(baseTangent));
    if (angle < 0.0f) {
        angle += 2.0f * Math::Pi;
    }

    constexpr float tangentLevels = static_cast<float>(1u << TangentBits);
    const uint32_t quantizedTangent = static_cast<uint32_t>(angle * (tangentLevels / (2.0f * Math::Pi)) + 0.5f) & TangentMask;
#endif
    const uint32_t signBit = bitangentSign < 0.0f ? 1u : 0u;

    return packedNormal | (quantizedTangent << TangentShift) | (signBit << BiTangentSignShift);
}

BE_INLINE void PackedTangentFrame::Unpack(uint32_t packed, Vec3 &normal, Vec3 &tangent, float &bitangentSign) {
    normal = UnpackNormal(packed);
    tangent = ReconstructTangent(packed, normal);
    bitangentSign = UnpackBiTangentSign(packed);
}

BE_INLINE Vec3 PackedTangentFrame::UnpackNormal(uint32_t packed) {
    constexpr float invNormalMask = 1.0f / static_cast<float>(NormalMask);
    const float x = static_cast<float>(packed & NormalMask) * invNormalMask * 2.0f - 1.0f;
    const float y = static_cast<float>((packed >> NormalYShift) & NormalMask) * invNormalMask * 2.0f - 1.0f;
    return Octahedral::Decode(Vec2(x, y));
}

BE_INLINE Vec3 PackedTangentFrame::UnpackTangent(uint32_t packed) {
    const Vec3 normal = UnpackNormal(packed);
    return ReconstructTangent(packed, normal);
}

BE_INLINE float PackedTangentFrame::UnpackBiTangentSign(uint32_t packed) {
    return (packed >> BiTangentSignShift) != 0u ? -1.0f : 1.0f;
}

BE_INLINE uint32_t PackedTangentFrame::ReplaceBiTangentSign(uint32_t packed, float bitangentSign) {
    constexpr uint32_t signMask = 1u << BiTangentSignShift;
    return (packed & ~signMask) | (bitangentSign < 0.0f ? signMask : 0u);
}

BE_INLINE Vec3 PackedTangentFrame::BuildBaseTangent(const Vec3 &normal) {
    Vec3 baseTangent;
    if (Math::Fabs(normal.x) > Math::Fabs(normal.z)) {
        baseTangent.Set(-normal.y, normal.x, 0.0f);
    } else {
        baseTangent.Set(0.0f, -normal.z, normal.y);
    }
    baseTangent.Normalize();
    return baseTangent;
}

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
BE_INLINE float PackedTangentFrame::EncodeDiamond(const Vec2 &direction) {
    const float l1Norm = Math::Fabs(direction.x) + Math::Fabs(direction.y);
    if (l1Norm <= 1e-20f) {
        return 0.0f;
    }

    const float diamondX = direction.x / l1Norm;
    const float ySign = direction.y >= 0.0f ? 1.0f : -1.0f;
    const float diamond = -ySign * 0.25f * diamondX + 0.5f + ySign * 0.25f;

    // Rotate the periodic coordinate so zero still represents baseTangent.
    const float encoded = diamond + 0.5f;
    return encoded >= 1.0f ? encoded - 1.0f : encoded;
}

BE_INLINE Vec2 PackedTangentFrame::DecodeDiamond(float encoded) {
    // Undo the coordinate rotation applied by EncodeDiamond.
    float diamond = encoded + 0.5f;
    if (diamond >= 1.0f) {
        diamond -= 1.0f;
    }

    const float diamondSign = diamond >= 0.5f ? 1.0f : -1.0f;
    const float x = -diamondSign * 4.0f * diamond + 1.0f + diamondSign * 2.0f;
    const float y = diamondSign * (1.0f - Math::Fabs(x));
    const float invLength = Math::InvSqrt(x * x + y * y);
    return Vec2(x * invLength, y * invLength);
}
#endif

BE_INLINE Vec3 PackedTangentFrame::ReconstructTangent(uint32_t packed, const Vec3 &normal) {
    const Vec3 baseTangent = BuildBaseTangent(normal);
    const Vec3 baseBiTangent = Vec3::FromCross(normal, baseTangent);
    const uint32_t quantizedTangent = (packed >> TangentShift) & TangentMask;
    constexpr float tangentLevels = static_cast<float>(1u << TangentBits);

#ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
    const Vec2 tangentDirection = DecodeDiamond(static_cast<float>(quantizedTangent) / tangentLevels);
    return baseTangent * tangentDirection.x + baseBiTangent * tangentDirection.y;
#else
    const float angle = static_cast<float>(quantizedTangent) * ((2.0f * Math::Pi) / tangentLevels);
    float sinAngle;
    float cosAngle;
    Math::SinCos(angle, sinAngle, cosAngle);
    return baseTangent * cosAngle + baseBiTangent * sinAngle;
#endif
}

BE_INLINE uint32_t PackedTangentFrame::QuantizeUNorm(float value, uint32_t maxValue) {
    value = Clamp01(value);
    return static_cast<uint32_t>(value * static_cast<float>(maxValue) + 0.5f);
}

BE_NAMESPACE_END
