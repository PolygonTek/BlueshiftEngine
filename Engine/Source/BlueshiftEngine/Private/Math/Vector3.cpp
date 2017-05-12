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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Vec3 Vec3::origin(0.0f, 0.0f, 0.0f);
const Vec3 Vec3::zero(0.0f, 0.0f, 0.0f);
const Vec3 Vec3::one(1.0f, 1.0f, 1.0f);
const Vec3 Vec3::unitX(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::unitY(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::unitZ(0.0f, 0.0f, 1.0f);

bool Vec3::FixDegenerateNormal() {
    if (x == 0.0f) {
        if (y == 0.0f) {
            if (z > 0.0f) {
                if (z != 1.0f) {
                    z = 1.0f;
                    return true;
                }
            } else {
                if (z != -1.0f)	{
                    z = -1.0f;
                    return true;
                }
            }
            return false;
        } else if (z == 0.0f)	{
            if (y > 0.0f) {
                if (y != 1.0f) {
                    y = 1.0f;
                    return true;
                }
            } else {
                if (y != -1.0f) {
                    y = -1.0f;
                    return true;
                }
            }
            return false;
        }
    } else if (y == 0.0f) {
        if (z == 0.0f) {
            if (x > 0.0f) {
                if (x != 1.0f) {
                    x = 1.0f;
                    return true;
                }
            } else {
                if (x != -1.0f)	{
                    x = -1.0f;
                    return true;
                }
            }
            return false;
        }
    }

    if (Math::Fabs(x) == 1.0f) {
        if (y != 0.0f || z != 0.0f) {
            y = z = 0.0f;
            return true;
        }
        return false;
    } else if (Math::Fabs(y) == 1.0f) {
        if (x != 0.0f || z != 0.0f) {
            x = z = 0.0f;
            return true;
        }
        return false;
    } else if (Math::Fabs(z) == 1.0f) {
        if (x != 0.0f || y != 0.0f)	{
            x = y = 0.0f;
            return true;
        }
        return false;
    }
    return false;
}

// Spherical linear interpolation from v1 to v2.
// Vectors are expected to be normalized.
#define LERP_DELTA 1e-6

void Vec3::SetFromSLerp(const Vec3 &v1, const Vec3 &v2, const float t) {
    if (t <= 0.0f) {
        (*this) = v1;
        return;
    } else if (t >= 1.0f) {
        (*this) = v2;
        return;
    }
    
    float scale0, scale1;
    float cosom = v1.Dot(v2);

    if ((1.0f - cosom) > LERP_DELTA) {
        float omega = acos(cosom);
        float sinom = sin(omega);
        scale0 = sin((1.0f - t) * omega) / sinom;
        scale1 = sin(t * omega) / sinom;
    } else {
        scale0 = 1.0f - t;
        scale1 = t;
    }

    (*this) = (v1 * scale0 + v2 * scale1);
}

Vec3 Vec3::FromUnitSphere(float u1, float u2) {
    float z = 1.0f - 2.0f * u1;
    float r = Math::Sqrt(1.0f - z * z);
    float s, c;
    Math::SinCos(Math::TwoPi * u2, s, c);
    return Vec3(r * c, r * s, z);
}

Vec3 Vec3::FromUnitHemisphere(float u1, float u2) {
    float r = Math::Sqrt(1.0f - u1 * u1);
    float s, c;
    Math::SinCos(Math::TwoPi * u2, s, c);
    return Vec3(r * c, r * s, u1);
}

float Vec3::ComputeYaw() const {
    float yaw;
    
    if (x == 0.0f && y == 0.0f) {
        yaw = 0.0f;
    } else {
        yaw = RAD2DEG(atan2(y, x));
        if (yaw < 0.0f) {
            yaw += 360.0f;
        }
    }

    return yaw;
}

float Vec3::ComputePitch() const {
    float   forward;
    float   pitch;
    
    if (x == 0.0f && y == 0.0f) {
        if (z > 0.0f) {
            pitch = 90.0f;
        } else {
            pitch = 270.0f;
        }
    } else {
        forward = (float)Math::Sqrt(x * x + y * y);
        pitch = RAD2DEG(atan2(z, forward));
        if (pitch < 0.0f) {
            pitch += 360.0f;
        }
    }

    return pitch;
}

Angles Vec3::ToAngles() const {
    float forward;
    float yaw;
    float pitch;
    
    if ((x == 0.0f) && (y == 0.0f)) {
        yaw = 0.0f;
        if (z > 0.0f) {
            pitch = 90.0f;
        } else {
            pitch = 270.0f;
        }
    } else {
        yaw = RAD2DEG(atan2(y, x));
        if (yaw < 0.0f) {
            yaw += 360.0f;
        }

        forward = (float)Math::Sqrt(x * x + y * y);
        pitch = RAD2DEG(atan2(z, forward));
        if (pitch < 0.0f) {
            pitch += 360.0f;
        }
    }

    return Angles(yaw, -pitch, 0.0f);
}

Mat3 Vec3::ToMat3() const {
    Mat3 mat;
    mat[0] = *this;

    float l = x * x + y * y;
    if (!l) {
        mat[1][0] = 1.0f;
        mat[1][1] = 0.0f;
        mat[1][2] = 0.0f;
    } else {
        float s = Math::InvSqrt(l);
        mat[1][0] = -y * s;
        mat[1][1] = x * s;
        mat[1][2] = 0.0f;
    }

    mat[2] = Cross(mat[1]);
    return mat;
}

const Vec3 Vec3::Compute3DBarycentricCoords(const Vec2 &s1, const Vec2 &s2, const Vec2 &s3, const Vec2 &p) {
    Vec2 a = s1 - s3;
    Vec2 b = s2 - s3;
    float det = a[0] * b[1] - b[0] * a[1];

    if (Math::Fabs(det) < Math::FloatEpsilon) {
        return Vec3::zero;
    }

    Vec2 c = p - s3;
    float b1 = (b[1] * c[0] - b[0] * c[1]) / det;
    float b2 = (a[0] * c[1] - a[1] * c[0]) / det;
    return Vec3(b1, b2, 1.0f - b1 - b2);
}

BE_NAMESPACE_END
