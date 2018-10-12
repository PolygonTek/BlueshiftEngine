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

/*
-------------------------------------------------------------------------------

    JointPose

-------------------------------------------------------------------------------
*/

class BE_API JointPose {
public:
                        operator const float *() const { return (const float *)q; }
                        operator float *() { return (float *)q; }

    JointPose &         operator=(const JointPose &other) = default;

    JointPose &         operator-=(const JointPose &baseJointPose);
    JointPose           operator-(const JointPose &baseJointPose) const;
    
    void                ClearQuat() { q[0] = q[1] = q[2] = q[3] = 0; }
    void                ClearTranslation() { t[0] = t[1] = t[2] = 0; }
    void                ClearScale() { s[0] = s[1] = s[2] = 1.0f; }

    bool                Compare(const JointPose &jointPose, float q_epsilon, float t_epsilon, float s_epsilon) const;

    void                SetFromMat3x4(const Mat3x4 &mat);

    Quat                q;  ///< Rotation in quaternion
    Vec3                t;  ///< Translation vector
    Vec3                s;  ///< Scaling vector for each axis
};

BE_INLINE bool JointPose::Compare(const JointPose &jointPose, float q_epsilon, float t_epsilon, float s_epsilon) const {
    return (q.Equals(jointPose.q, q_epsilon) && t.Equals(jointPose.t, t_epsilon) && s.Equals(jointPose.s, s_epsilon)) ? true : false;
} 

BE_INLINE JointPose &JointPose::operator-=(const JointPose &baseJointPose) {
    // Q2 * Q1 = Q3
    // Q2 = Q3 * Q1^-1
    q = q * baseJointPose.q.Inverse();
    if (q.w < 0) {
        q = -q;
    }
    t = t - baseJointPose.t;
    s = s / baseJointPose.s;
    return *this;
}

BE_INLINE JointPose JointPose::operator-(const JointPose &baseJointPose) const {
    JointPose delta;
    delta.q = q * baseJointPose.q.Inverse();
    if (delta.q.w < 0) {
        delta.q = -delta.q;
    }
    delta.t = t - baseJointPose.t;
    delta.s = s / baseJointPose.s;
    return delta;
}

BE_INLINE void JointPose::SetFromMat3x4(const Mat3x4 &mat) {
    this->s[0] = Math::Sqrt(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0]);
    this->s[1] = Math::Sqrt(mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1]);
    this->s[2] = Math::Sqrt(mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2]);

    this->t[0] = mat[0][3];
    this->t[1] = mat[1][3];
    this->t[2] = mat[2][3];

    // column major order 3x3 matrix that has rotation only
    Vec3 invS = 1.0f / this->s;
    Mat3 r;
    r[0][0] = mat[0][0] * invS.x;
    r[0][1] = mat[1][0] * invS.x;
    r[0][2] = mat[2][0] * invS.x;
    r[1][0] = mat[0][1] * invS.y;
    r[1][1] = mat[1][1] * invS.y;
    r[1][2] = mat[2][1] * invS.y;
    r[2][0] = mat[0][2] * invS.z;
    r[2][1] = mat[1][2] * invS.z;
    r[2][2] = mat[2][2] * invS.z;

    this->q = r.ToQuat();
}

/*
-------------------------------------------------------------------------------

    CompressedJointPose

-------------------------------------------------------------------------------
*/

class BE_API CompressedJointPose {
public:
    static const int    MaxBoneTranslation = MeterToUnit(4);
    static const int    MaxBoneScale = 2;

    static short        QuatToShort(const float c);
    static float        ShortToQuat(const short s);
    static short        TranslationToShort(const float c);
    static float        ShortToTranslation(const short s);
    static short        ScaleToShort(const float c);
    static float        ShortToScale(const short s);

    void                ClearQuat() { q[0] = q[1] = q[2] = 0; }
    void                ClearTranslation() { t[0] = t[1] = t[2] = 0; }
    void                ClearScale() { s[0] = s[1] = s[2] = ScaleToShort(1.0f); }

    Quat                ToQuat() const;
    Vec3                ToTranslation() const;
    Vec3                ToScale() const;

    short               q[3];
    short               t[3];
    short               s[3];
};

BE_INLINE short CompressedJointPose::QuatToShort(const float x) {
    return ClampShort(Math::Ftoi((x < 0) ? (x * 32767.0f - 0.5f) : (x * 32767.0f + 0.5f)));
}

BE_INLINE float CompressedJointPose::ShortToQuat(const short x) {
    return x * (1.0f / 32767.0f);
}

BE_INLINE short CompressedJointPose::TranslationToShort(const float x) {
    assert(x < MaxBoneTranslation && x > -MaxBoneTranslation);
    return ClampShort(Math::Ftoi((x < 0) ? (x * (32767.0f / MaxBoneTranslation) - 0.5f) : (x * (32767.0f / MaxBoneTranslation) + 0.5f)));
}

BE_INLINE float CompressedJointPose::ShortToTranslation(const short x) {
    return x * (1.0f / (32767.0f / MaxBoneTranslation));
}

BE_INLINE short CompressedJointPose::ScaleToShort(const float x) {
    return ClampShort(Math::Ftoi((x < 0) ? (x * 32767.0f / MaxBoneScale - 0.5f) : (x * 32767.0f / MaxBoneScale + 0.5f)));
}

BE_INLINE float CompressedJointPose::ShortToScale(const short x) {
    return x * (1.0f / (32767.0f / MaxBoneScale));
}

BE_INLINE Quat CompressedJointPose::ToQuat() const {
    Quat quat;
    quat.x = ShortToQuat(q[0]);
    quat.y = ShortToQuat(q[1]);
    quat.z = ShortToQuat(q[2]);
    // take the absolute value because floating point rounding may cause the dot of x, y, z to be larger than 1
    quat.w = Math::Sqrt(Math::Fabs(1.0f - (quat.x * quat.x + quat.y * quat.y + quat.z * quat.z)));
    return quat;
}

BE_INLINE Vec3 CompressedJointPose::ToTranslation() const {
    return Vec3(ShortToTranslation(t[0]), ShortToTranslation(t[1]), ShortToTranslation(t[2]));
}

BE_INLINE Vec3 CompressedJointPose::ToScale() const {
    return Vec3(ShortToScale(t[0]), ShortToScale(t[1]), ShortToScale(t[2]));
}

BE_NAMESPACE_END
