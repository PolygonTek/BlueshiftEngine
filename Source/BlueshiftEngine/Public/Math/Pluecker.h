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

/*
-------------------------------------------------------------------------------

    Pluecker coordinate

    A(x, y, z, w) - B(x, y, z, w)

    P1 = Ax * By - Bx * Ay
    P2 = Ax * Bz - Bx * Az
    P3 = Ax * Bw - Bx * Aw
    P4 = Ay * Bz - By * Az
    P5 = Az * Bw - Bz * Aw
    P6 = Aw * By - Bw * Ay
  
    P1 * P6 - P2 * P5 + P3 * P4 = 0

    참고: http://www.acm.org/tog/resources/RTNews/html/rtnv10n3.html#art11

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Pluecker {
public:
    /// The default constructor does not initialize any members of this class.
    Pluecker() {}
    Pluecker(const Vec3 &start, const Vec3 &end);
    Pluecker(const float a1, const float a2, const float a3, const float a4, const float a5, const float a6);
    explicit Pluecker(const float *a);

    const float *           Ptr() const { return p; }
    float *                 Ptr() { return p; }

                            operator const float *() const { return p; }
                            operator float *() { return p; }

    float                   operator[](const int index) const;
    float &                 operator[](const int index);

                            /// Flips the direction
    Pluecker                operator-() const;

    Pluecker                operator*(const float rhs) const;
    Pluecker                operator/(const float rhs) const;
    
                            // permuted inner product
    float                   operator*(const Pluecker &rhs) const;
    Pluecker                operator-(const Pluecker &rhs) const;
    Pluecker                operator+(const Pluecker &rhs) const;
    Pluecker &              operator*=(const float rhs);
    Pluecker &              operator/=(const float rhs);
    Pluecker &              operator+=(const Pluecker &rhs);
    Pluecker &              operator-=(const Pluecker &rhs);

                            /// Exact compare, no epsilon
    bool                    Equals(const Pluecker &a) const;
                            /// Compare with epsilon
    bool                    Equals(const Pluecker &a, const float epsilon) const;
                            /// Exact compare, no epsilon
    bool                    operator==(const Pluecker &rhs) const { return Equals(rhs); }
                            /// Exact compare, no epsilon
    bool                    operator!=(const Pluecker &rhs) const { return !Equals(rhs); }

    void                    Set(const float a1, const float a2, const float a3, const float a4, const float a5, const float a6);
    void                    SetZero();

    void                    SetFromLine(const Vec3 &start, const Vec3 &end);
    void                    SetFromRay(const Vec3 &start, const Vec3 &dir);
    bool                    SetFromPlanes(const Plane &p1, const Plane &p2);

    bool                    ToLine(Vec3 &start, Vec3 &end) const;
    bool                    ToRay(Vec3 &start, Vec3 &dir) const;
    void                    ToDir(Vec3 &dir) const;
    
    float                   PermutedInnerProduct(const Pluecker &a) const;
    float                   Distance3DSqr(const Pluecker &a) const;

    float                   Length() const;
    float                   LengthSqr() const;
    Pluecker                Normalize() const;
    float                   NormalizeSelf();

                            /// Returns dimension of this type
    int                     GetDimension() const { return 6; }

    static const Pluecker   origin;
    static const Pluecker   zero;
    
private:
    float                   p[6];
};

BE_INLINE Pluecker::Pluecker(const float *a) {
    memcpy(p, a, 6 * sizeof(float));
}

BE_INLINE Pluecker::Pluecker(const Vec3 &start, const Vec3 &end) {
    SetFromLine(start, end);
}

BE_INLINE Pluecker::Pluecker(const float a1, const float a2, const float a3, const float a4, const float a5, const float a6) {
    p[0] = a1;
    p[1] = a2;
    p[2] = a3;
    p[3] = a4;
    p[4] = a5;
    p[5] = a6;
}

BE_INLINE Pluecker Pluecker::operator-() const {
    return Pluecker(-p[0], -p[1], -p[2], -p[3], -p[4], -p[5]);
}

BE_INLINE float Pluecker::operator[](const int index) const {
    return p[index];
}

BE_INLINE float &Pluecker::operator[](const int index) {
    return p[index];
}

BE_INLINE Pluecker Pluecker::operator*(const float rhs) const {
    return Pluecker(p[0] * rhs, p[1] * rhs, p[2] * rhs, p[3] * rhs, p[4] * rhs, p[5] * rhs);
}

BE_INLINE float Pluecker::operator*(const Pluecker &rhs) const {
    return p[0] * rhs.p[4] + p[1] * rhs.p[5] + p[2] * rhs.p[3] + p[4] * rhs.p[0] + p[5] * rhs.p[1] + p[3] * rhs.p[2];
}

BE_INLINE Pluecker Pluecker::operator/(const float rhs) const {
    assert(rhs != 0.0f);
    float inva = 1.0f / rhs;
    return Pluecker(p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva);
}

BE_INLINE Pluecker Pluecker::operator+(const Pluecker &rhs) const {
    return Pluecker(p[0] + rhs[0], p[1] + rhs[1], p[2] + rhs[2], p[3] + rhs[3], p[4] + rhs[4], p[5] + rhs[5]);
}

BE_INLINE Pluecker Pluecker::operator-(const Pluecker &rhs) const {
    return Pluecker(p[0] - rhs[0], p[1] - rhs[1], p[2] - rhs[2], p[3] - rhs[3], p[4] - rhs[4], p[5] - rhs[5]);
}

BE_INLINE Pluecker &Pluecker::operator*=(const float rhs) {
    p[0] *= rhs;
    p[1] *= rhs;
    p[2] *= rhs;
    p[3] *= rhs;
    p[4] *= rhs;
    p[5] *= rhs;
    return *this;
}

BE_INLINE Pluecker &Pluecker::operator/=(const float rhs) {
    assert(rhs != 0.0f);
    float inva = 1.0f / rhs;
    p[0] *= inva;
    p[1] *= inva;
    p[2] *= inva;
    p[3] *= inva;
    p[4] *= inva;
    p[5] *= inva;
    return *this;
}

BE_INLINE Pluecker &Pluecker::operator+=(const Pluecker &rhs) {
    p[0] += rhs[0];
    p[1] += rhs[1];
    p[2] += rhs[2];
    p[3] += rhs[3];
    p[4] += rhs[4];
    p[5] += rhs[5];
    return *this;
}

BE_INLINE Pluecker &Pluecker::operator-=(const Pluecker &rhs) {
    p[0] -= rhs[0];
    p[1] -= rhs[1];
    p[2] -= rhs[2];
    p[3] -= rhs[3];
    p[4] -= rhs[4];
    p[5] -= rhs[5];
    return *this;
}

BE_INLINE bool Pluecker::Equals(const Pluecker &a) const {
    return ((p[0] == a[0]) && (p[1] == a[1]) && (p[2] == a[2]) &&
            (p[3] == a[3]) && (p[4] == a[4]) && (p[5] == a[5]));
}

BE_INLINE bool Pluecker::Equals(const Pluecker &a, const float epsilon) const {
    if (Math::Fabs(p[0] - a[0]) > epsilon) return false;
    if (Math::Fabs(p[1] - a[1]) > epsilon) return false;
    if (Math::Fabs(p[2] - a[2]) > epsilon) return false;
    if (Math::Fabs(p[3] - a[3]) > epsilon) return false;	
    if (Math::Fabs(p[4] - a[4]) > epsilon) return false;	
    if (Math::Fabs(p[5] - a[5]) > epsilon) return false;	
    return true;
}

BE_INLINE void Pluecker::Set(const float a1, const float a2, const float a3, const float a4, const float a5, const float a6) {
    p[0] = a1;
    p[1] = a2;
    p[2] = a3;
    p[3] = a4;
    p[4] = a5;
    p[5] = a6;
}

BE_INLINE void Pluecker::SetZero() {
    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

BE_INLINE void Pluecker::SetFromLine(const Vec3 &start, const Vec3 &end) {
    p[0] = start[0] * end[1] - end[0] * start[1];
    p[1] = start[0] * end[2] - end[0] * start[2];
    p[2] = start[0] - end[0];
    p[3] = start[1] * end[2] - end[1] * start[2];
    p[4] = start[2] - end[2];
    p[5] = end[1] - start[1];
}

BE_INLINE void Pluecker::SetFromRay(const Vec3 &start, const Vec3 &dir) {
    p[0] = start[0] * dir[1] - dir[0] * start[1];
    p[1] = start[0] * dir[2] - dir[0] * start[2];
    p[2] = -dir[0];
    p[3] = start[1] * dir[2] - dir[1] * start[2];
    p[4] = -dir[2];
    p[5] = dir[1];
}

BE_INLINE bool Pluecker::ToLine(Vec3 &start, Vec3 &end) const {
    Vec3 dir1, dir2;

    dir1[0] = p[3];
    dir1[1] = -p[1];
    dir1[2] = p[0];

    dir2[0] = -p[2];
    dir2[1] = p[5];
    dir2[2] = -p[4];

    float d = dir2.Dot(dir2);
    if (d == 0.0f) {
        return false; // pluecker coordinate does not represent a line
    }
    
    start = dir2.Cross(dir1) * (1.0f / d);
    end = start + dir2;
    return true;
}

BE_INLINE bool Pluecker::ToRay(Vec3 &start, Vec3 &dir) const {
    Vec3 dir1;
    
    dir1[0] = p[3];
    dir1[1] = -p[1];
    dir1[2] = p[0];

    dir[0] = -p[2];
    dir[1] = p[5];
    dir[2] = -p[4];

    float d = dir.Dot(dir);
    if (d == 0.0f) {
        return false; // pluecker coordinate does not represent a line
    }

    start = dir.Cross(dir1) * (1.0f / d);
    return true;
}

BE_INLINE void Pluecker::ToDir(Vec3 &dir) const {
    dir[0] = -p[2];
    dir[1] = p[5];
    dir[2] = -p[4];
}

BE_INLINE float Pluecker::PermutedInnerProduct(const Pluecker &a) const {
    return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

BE_INLINE float Pluecker::Length() const {
    return (float)Math::Sqrt(p[5] * p[5] + p[4] * p[4] + p[2] * p[2]);
}

BE_INLINE float Pluecker::LengthSqr() const {
    return (p[5] * p[5] + p[4] * p[4] + p[2] * p[2]);
}

BE_INLINE float Pluecker::NormalizeSelf() {
    float l = LengthSqr();
    if (l == 0.0f) {
        return l; // pluecker coordinate does not represent a line
    }
    float d = Math::InvSqrt(l);
    p[0] *= d;
    p[1] *= d;
    p[2] *= d;
    p[3] *= d;
    p[4] *= d;
    p[5] *= d;
    return d * l;
}

BE_INLINE Pluecker Pluecker::Normalize() const {
    float d = LengthSqr();
    if (d == 0.0f) {
        return *this; // pluecker coordinate does not represent a line
    }
    d = Math::InvSqrt(d);
    return Pluecker(p[0]*d, p[1]*d, p[2]*d, p[3]*d, p[4]*d, p[5]*d);
}

BE_NAMESPACE_END
