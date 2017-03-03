#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Vec2 Vec2::origin(0.0f, 0.0f);
const Vec2 Vec2::zero(0.0f, 0.0f);
const Vec2 Vec2::one(1.0f, 1.0f);
const Vec2 Vec2::unitX(1.0f, 0.0f);
const Vec2 Vec2::unitY(0.0f, 1.0f);

const Vec2 Vec2::Compute2DBarycentricCoords(const float s1, const float s2, const float p) {
    float a = s1 - s2;
    if (Math::Fabs(a) < Math::FloatEpsilon) {
        return Vec2::zero;
    }
    float b = (p - s2) / a;

    return Vec2(b, 1.0f - b);
}

BE_NAMESPACE_END
