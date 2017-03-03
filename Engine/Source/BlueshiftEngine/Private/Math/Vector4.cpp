#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Vec4 Vec4::origin(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::zero(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::one(1.0f, 1.0f, 1.0f, 1.0f);
const Vec4 Vec4::unitX(1.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::unitY(0.0f, 1.0f, 0.0f, 0.0f);
const Vec4 Vec4::unitZ(0.0f, 0.0f, 1.0f, 0.0f);
const Vec4 Vec4::unitW(0.0f, 0.0f, 0.0f, 1.0f);

const Vec4 Vec4::Compute4DBarycentricCoords(const Vec3 &s1, const Vec3 &s2, const Vec3 &s3, const Vec3 &s4, const Vec3 &p) {
    Mat3 matInv(s1 - s4, s2 - s4, s3 - s4);
    if (!matInv.InverseSelf()) {
        return Vec4::zero;
    }
    Vec3 b = matInv * (p - s4);

    return Vec4(b[0], b[1], b[2], 1.0f - b[0] - b[1] - b[2]);
}

BE_NAMESPACE_END
