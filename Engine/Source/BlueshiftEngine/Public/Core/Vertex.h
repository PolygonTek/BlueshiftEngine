// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

// The hardware converts a byte to a float by division with 255 and in the
// vertex programs we convert the floating-point value in the range [0, 1]
// to the range [-1, 1] by multiplying with 2 and subtracting 1.
#define BYTE_TO_SIGNED_FLOAT(x)         ((x) * (2.0f / 255.0f) - 1.0f)
#define SIGNED_FLOAT_TO_BYTE(x)         Math::Ftob(((x) + 1.0f) * (255.0f / 2.0f) + 0.5f)

#if 1
typedef uint32_t TriIndex;
#else
typedef uint16_t TriIndex;
#endif

// this is used for calculating unsmoothed normals and tangents for deformed models
struct DominantTri {
    TriIndex        v2, v3;                 // dominant triangle index [i, v2, v3]
    float           normalizationScale[3];  // tangent, bitangent, normal 의 normalization scale
};

/*
-------------------------------------------------------------------------------

    VertexGeneric

-------------------------------------------------------------------------------
*/

struct BE_API VertexGeneric {
    Vec3            xyz;
    float16_t       st[2];
    byte            color[4];

    void            Clear();

    float           operator[](const int index) const { return ((float *)(&xyz))[index]; }
    float &         operator[](const int index) { return ((float *)(&xyz))[index]; }

    const Vec3      GetPosition() const { return xyz; }

    void            SetPosition(const Vec3 &p) { xyz = p; }
    void            SetPosition(float x, float y, float z) { xyz.Set(x, y, z); }

    const Vec2      GetTexCoord() const;
    void            SetTexCoord(const Vec2 &st);
    void            SetTexCoord(float s, float t);
    void            SetTexCoordS(float s);
    void            SetTexCoordT(float t);

    void            SetColor(uint32_t color);
    uint32_t        GetColor() const;

    void            Lerp(const VertexGeneric &a, const VertexGeneric &b, const float f);

    void            Transform(const Mat3 &rotation, const Vec3 &translation);
};

BE_INLINE void VertexGeneric::Clear() {
    SetPosition(0, 0, 0);
    SetTexCoord(0, 0);
    SetColor(0);
}

BE_INLINE const Vec2 VertexGeneric::GetTexCoord() const {
    return Vec2(F16Converter::ToF32(st[0]), F16Converter::ToF32(st[1]));
}

BE_INLINE void VertexGeneric::SetTexCoord(const Vec2 &st) {
    SetTexCoordS(st.x);
    SetTexCoordT(st.y);
}

BE_INLINE void VertexGeneric::SetTexCoord(float s, float t) {
    SetTexCoordS(s);
    SetTexCoordT(t);
}

BE_INLINE void VertexGeneric::SetTexCoordS(float s) {
    st[0] = F16Converter::FromF32(s);
}

BE_INLINE void VertexGeneric::SetTexCoordT(float t) {
    st[1] = F16Converter::FromF32(t);
}

BE_INLINE void VertexGeneric::SetColor(uint32_t color) {
    *reinterpret_cast<uint32_t *>(this->color) = color;
}

BE_INLINE uint32_t VertexGeneric::GetColor() const {
    return *reinterpret_cast<const uint32_t *>(this->color);
}

BE_INLINE void VertexGeneric::Lerp(const VertexGeneric &a, const VertexGeneric &b, const float f) {
    xyz = BE1::Lerp(a.xyz, b.xyz, f);
    SetTexCoord(BE1::Lerp(a.GetTexCoord(), b.GetTexCoord(), f));

    color[0] = (byte)(a.color[0] + f * (b.color[0] - a.color[0]));
    color[1] = (byte)(a.color[1] + f * (b.color[1] - a.color[1]));
    color[2] = (byte)(a.color[2] + f * (b.color[2] - a.color[2]));
    color[3] = (byte)(a.color[3] + f * (b.color[3] - a.color[3]));
}

BE_INLINE void VertexGeneric::Transform(const Mat3 &rotation, const Vec3 &translation) {
    Mat4 matrix(rotation, translation);
    xyz = matrix * xyz;
}

/*
-------------------------------------------------------------------------------

    VertexGenericLit

-------------------------------------------------------------------------------
*/

#define COMPRESSED_VERTEX_NORMAL_TANGENTS

struct BE_API VertexGenericLit : public VertexGeneric {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    byte            normal[4];
    byte            tangent[4];
#else
    float16_t       normal[4];
    float16_t       tangent[4];
#endif
    
    void            Clear();
    
    const Vec3      GetNormal() const;
    const Vec3      GetNormalRaw() const;
    
                    // must be normalized already!
    void            SetNormal(const Vec3 &n);
    void            SetNormal(float x, float y, float z);

    const Vec3      GetTangent() const;
    const Vec3      GetTangentRaw() const;

                    // must be normalized already!
    void            SetTangent(const Vec3 &n);
    void            SetTangent(float x, float y, float z);

                    // derived from normal, tangent, and tangent flag
    const Vec3      GetBiTangent() const;
    const Vec3      GetBiTangentRaw() const;

    void            SetBiTangent(float x, float y, float z);
    void            SetBiTangent(const Vec3 & t);

    float           GetBiTangentSign() const;

                    // either 1.0f or -1.0f
    void            SetBiTangentSign(float sign);

    void            Lerp(const VertexGenericLit &a, const VertexGenericLit &b, const float f);

    void            Transform(const Mat3 &rotation, const Vec3 &translation);
};

BE_INLINE void ConvertNormalToBytes(const float &x, const float &y, const float &z, byte *bval) {
    assert((((uintptr_t)bval) & 3) == 0);

#if BE_WIN_X86_SSE_INTRIN
    const __m128 vector_float_one           = { 1.0f, 1.0f, 1.0f, 1.0f };
    const __m128 vector_float_half          = { 0.5f, 0.5f, 0.5f, 0.5f };
    const __m128 vector_float_255_over_2    = { 255.0f / 2.0f, 255.0f / 2.0f, 255.0f / 2.0f, 255.0f / 2.0f };

    const __m128 xyz = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&z)), _mm_load_ss(&y));
    const __m128 xyzScaled = _mm_add_ps(_mm_mul_ps(_mm_add_ps(xyz, vector_float_one), vector_float_255_over_2), vector_float_half);
    const __m128i xyzInt = _mm_cvtps_epi32(xyzScaled);
    const __m128i xyzShort = _mm_packs_epi32(xyzInt, xyzInt);
    const __m128i xyzChar = _mm_packus_epi16(xyzShort, xyzShort);
    const __m128i xyz16 = _mm_unpacklo_epi8(xyzChar, _mm_setzero_si128());

    bval[0] = (byte)_mm_extract_epi16(xyz16, 0);    // cannot use _mm_extract_epi8 because it is an SSE4 instruction
    bval[1] = (byte)_mm_extract_epi16(xyz16, 1);
    bval[2] = (byte)_mm_extract_epi16(xyz16, 2);
#else
    bval[0] = SIGNED_FLOAT_TO_BYTE(x);
    bval[1] = SIGNED_FLOAT_TO_BYTE(y);
    bval[2] = SIGNED_FLOAT_TO_BYTE(z);
#endif
}

BE_INLINE void VertexGenericLit::Clear() {
    VertexGeneric::Clear();
    SetNormal(0, 0, 1);
    SetTangent(1, 0, 0);
}

BE_INLINE const Vec3 VertexGenericLit::GetNormal() const {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Vec3 n(BYTE_TO_SIGNED_FLOAT(normal[0]), BYTE_TO_SIGNED_FLOAT(normal[1]), BYTE_TO_SIGNED_FLOAT(normal[2]));
    n.Normalize();
    return n;
#else
    Vec3 n(F16Converter::ToF32(normal[0]), F16Converter::ToF32(normal[1]), F16Converter::ToF32(normal[2]));
    n.Normalize();
    return n;
#endif
}

BE_INLINE const Vec3 VertexGenericLit::GetNormalRaw() const {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Vec3 n(BYTE_TO_SIGNED_FLOAT(normal[0]), BYTE_TO_SIGNED_FLOAT(normal[1]), BYTE_TO_SIGNED_FLOAT(normal[2]));
    return n;
#else
    Vec3 n(F16Converter::ToF32(normal[0]), F16Converter::ToF32(normal[1]), F16Converter::ToF32(normal[2]));
    return n;
#endif
}

BE_INLINE void VertexGenericLit::SetNormal(const Vec3 &n) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    ConvertNormalToBytes(n.x, n.y, n.z, normal);
#else
    normal[0] = F16Converter::FromF32(n[0]);
    normal[1] = F16Converter::FromF32(n[1]);
    normal[2] = F16Converter::FromF32(n[2]);
#endif
}

BE_INLINE void VertexGenericLit::SetNormal(float x, float y, float z) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    ConvertNormalToBytes(x, y, z, normal);
#else
    normal[0] = F16Converter::FromF32(x);
    normal[1] = F16Converter::FromF32(y);
    normal[2] = F16Converter::FromF32(z); 
#endif
}

BE_INLINE void VertexGenericLit::SetTangent(const Vec3 &t) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    ConvertNormalToBytes(t.x, t.y, t.z, tangent);
#else
    tangent[0] = F16Converter::FromF32(t[0]);
    tangent[1] = F16Converter::FromF32(t[1]);
    tangent[2] = F16Converter::FromF32(t[2]);
#endif
}

BE_INLINE void VertexGenericLit::SetTangent(float x, float y, float z) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    ConvertNormalToBytes(x, y, z, tangent);
#else
    tangent[0] = F16Converter::FromF32(x);
    tangent[1] = F16Converter::FromF32(y);
    tangent[2] = F16Converter::FromF32(z);
#endif
}

BE_INLINE const Vec3 VertexGenericLit::GetTangent() const {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Vec3 t(BYTE_TO_SIGNED_FLOAT(tangent[0]), BYTE_TO_SIGNED_FLOAT(tangent[1]), BYTE_TO_SIGNED_FLOAT(tangent[2]));
    t.Normalize();
    return t;
#else
    Vec3 t(F16Converter::ToF32(tangent[0]), F16Converter::ToF32(tangent[1]), F16Converter::ToF32(tangent[2]));
    t.Normalize();
    return t;
#endif
}

BE_INLINE const Vec3 VertexGenericLit::GetTangentRaw() const {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Vec3 t(BYTE_TO_SIGNED_FLOAT(tangent[0]), BYTE_TO_SIGNED_FLOAT(tangent[1]), BYTE_TO_SIGNED_FLOAT(tangent[2]));
    return t;
#else
    Vec3 t(F16Converter::ToF32(tangent[0]), F16Converter::ToF32(tangent[1]), F16Converter::ToF32(tangent[2]));
    return t;
#endif
}

BE_INLINE const Vec3 VertexGenericLit::GetBiTangent() const {
    // derive from the normal, tangent, and bitangent direction flag
    Vec3 bitangent = Vec3::FromCross(GetNormal(), GetTangent());
    bitangent *= GetBiTangentSign();
    return bitangent;
}

BE_INLINE const Vec3 VertexGenericLit::GetBiTangentRaw() const {
    // derive from the normal, tangent, and bitangent direction flag
    // don't re-normalize just like we do in the vertex programs
    Vec3 bitangent = Vec3::FromCross(GetNormalRaw(), GetTangentRaw());
    bitangent *= GetBiTangentSign();
    return bitangent;
}

BE_INLINE void VertexGenericLit::SetBiTangent(float x, float y, float z) {
    SetBiTangent(Vec3(x, y, z));
}

BE_INLINE void VertexGenericLit::SetBiTangent(const Vec3 &t) {
    Vec3 bitangent = Vec3::FromCross(GetNormal(), GetTangent());
    SetBiTangentSign(bitangent.Dot(t));
}

BE_INLINE float VertexGenericLit::GetBiTangentSign() const {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    return (tangent[3] < 128) ? -1.0f : 1.0f;
#else
    return (F16Converter::ToF32(tangent[3]) < 0.0f) ? -1.0f : 1.0f;
#endif
}

BE_INLINE void VertexGenericLit::SetBiTangentSign(float sign) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    tangent[3] = (sign < 0.0f) ? 0 : 255;
#else
    tangent[3] = F16Converter::FromF32((sign < 0.0f) ? -1.0f : 1.0f);
#endif
}

BE_INLINE void VertexGenericLit::Lerp(const VertexGenericLit &a, const VertexGenericLit &b, const float f) {
    VertexGeneric::Lerp(a, b, f);

    Vec3 normal = BE1::Lerp(a.GetNormal(), b.GetNormal(), f);
    Vec3 tangent = BE1::Lerp(a.GetTangent(), b.GetTangent(), f);
    Vec3 bitangent = BE1::Lerp(a.GetBiTangent(), b.GetBiTangent(), f);
    
    normal.Normalize();
    tangent.Normalize();
    bitangent.Normalize();

    SetNormal(normal);
    SetTangent(tangent);
    SetBiTangent(bitangent);
}

BE_INLINE void VertexGenericLit::Transform(const Mat3 &rotation, const Vec3 &translation) {
    Mat4 matrix(rotation, translation);
    xyz = matrix * xyz;
    SetNormal(rotation * GetNormal());
    SetTangent(rotation * GetTangent());
}

/*
-------------------------------------------------------------------------------

    VertexWeightX : vertex weight for HW skinning

-------------------------------------------------------------------------------
*/

#if 1
typedef byte JointWeightType;
#else
typedef float JointWeightType;
#endif

struct VertexWeight1 {
    uint32_t            jointIndex;
};
 
struct VertexWeight4 {
    byte                jointIndexes[4];
    JointWeightType     jointWeights[4];
};

struct VertexWeight8 {
    byte                jointIndexes[8];
    JointWeightType     jointWeights[8];
};

BE_NAMESPACE_END
