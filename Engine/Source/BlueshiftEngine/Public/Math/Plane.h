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

    Plane

    a*x + b*y + c*z + d = 0 ( d = - a*x0 - b*y0 - c*z0 )

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

#define ON_EPSILON                  0.1f
#define DEGENERATE_DIST_EPSILON     1e-4f

/// Surface in three-dimensional space.
class BE_API Plane {
public:
    /// Plane type
    enum Type {
        X = 0,
        Y = 1,
        Z = 2,
        NegX = 3,
        NegY = 4,
        NegZ = 5,
        TrueAxial = 6,    // all types < 6 are true axial planes
        ZeroX = 6,
        ZeroY = 7,
        ZeroZ = 8,
        NonAxial = 9
    };

    /// Plane sides
    enum Side {
        Front = 0,
        Back = 1,
        On = 2,
        Cross = 3
    };

    /// The default constructor does not initialize any members of this class.
    Plane() = default;
    Plane(float a, float b, float c, float d);
    Plane(const Vec3 &n, float d);

    void            SetNormal(const Vec3 &normal);
    void            SetDist(const float dist);

    int             GetType() const;
    
                    /// Casts this Plane to a C array.
                    /// This function simply returns a C pointer view to this data structure.
    const float *   Ptr() const { return (const float *)&a; }
    float *         Ptr() { return (float *)&a; }
                    /// Casts this Plane to a C array.
                    /// This function simply returns a C pointer view to this data structure.
                    /// This function is identical to the member function Ptr().
                    operator const float *() const { return (const float *)&a; }
                    operator float *() { return (float *)&a; }

                    /// Accesses an element of this plane.
    float &         At(int index) { return (*this)[index]; }
                    /// Accesses an element of this plane using array notation.
    float           operator[](int index) const;
    float &         operator[](int index);

    const Vec3 &    Normal() const;
    Vec3 &          Normal();

                    // returns: -d
    float           Dist() const;

    bool            SetFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, bool fixDegenerate = true);
    bool            SetFromVecs(const Vec3 &dir1, const Vec3 &dir2, const Vec3 &p, bool fixDegenerate = true);
                    
                    // assumes normal is valid
    void            FitThroughPoint(const Vec3 &p);
    void            Flip();

                    /// Normalize plane. 
                    /// Only normalizes the plane normal, does not adjust d
    float           Normalize(bool fixDegenerate = true);

    bool            FixDegenerateNormal();

    Plane           Translate(const Vec3 &translation) const;
    Plane &         TranslateSelf(const Vec3 &translation);

    Plane           Rotate(const Vec3 &origin, const Mat3 &axis) const;
    Plane &         RotateSelf(const Vec3 &origin, const Mat3 &axis);

    float           Distance(const Vec3 &v) const;
    int             GetSide(const Vec3 &v, const float epsilon) const;

    float           RayIntersection(const Vec3 &start, const Vec3 &dir) const;
    bool            LineIntersection(const Vec3 &start, const Vec3 &end) const;

                    /// Returns "a b c d".
    const char *    ToString() const { return ToString(4); }
                    /// Returns "a b c d" with the given precision.
    const char *    ToString(int precision) const;

                    /// Returns dimension of this type
    int             GetDimension() const { return 4; }

    float           a;
    float           b;
    float           c;
    float           d;
};

BE_INLINE Plane::Plane(float a, float b, float c, float d) {
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
}

BE_INLINE Plane::Plane(const Vec3 &n, float d) {
    this->a = n.x;
    this->b = n.y;
    this->c = n.z;
    this->d = d;
}

BE_INLINE float Plane::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return (&a)[index];
}

BE_INLINE float &Plane::operator[](int index) {
    assert(index >= 0 && index < 4);
    return (&a)[index];
}

BE_INLINE const Vec3 &Plane::Normal() const {
    return *reinterpret_cast<const Vec3 *>(&a);
}

BE_INLINE Vec3 &Plane::Normal() {
    return *reinterpret_cast<Vec3 *>(&a);
}

BE_INLINE float Plane::Dist() const {
    return -d;
}

BE_INLINE void Plane::SetNormal(const Vec3 &normal) {
    this->a = normal.x;
    this->b = normal.y;
    this->c = normal.z;
}

BE_INLINE void Plane::SetDist(const float dist) {
    d = -dist;
}

BE_INLINE bool Plane::SetFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, bool fixDegenerate) {
    Normal() = (p1 - p2).Cross(p3 - p2);
    if (Normalize(fixDegenerate) == 0.0f) {
        return false;
    }

    d = -Normal().Dot(p2);
    return true;
}

BE_INLINE bool Plane::SetFromVecs(const Vec3 &dir1, const Vec3 &dir2, const Vec3 &p, bool fixDegenerate) {
    Normal() = dir1.Cross(dir2);
    if (Normalize(fixDegenerate) == 0.0f) {
        return false;
    }

    d = -Normal().Dot(p);
    return true;
}


BE_INLINE void Plane::FitThroughPoint(const Vec3 &p) {
    d = -(Normal().Dot(p));
}

BE_INLINE void Plane::Flip() {
    a = -a;
    b = -b;
    c = -c;
    d = -d;
}

BE_INLINE float Plane::Normalize(bool fixDegenerate) {
    float length = Normal().Normalize();

    if (fixDegenerate) {
        FixDegenerateNormal();
    }

    return length;
}

BE_INLINE bool Plane::FixDegenerateNormal() {
    return Normal().FixDegenerateNormal();
}

BE_INLINE Plane Plane::Translate(const Vec3 &translation) const {
    return Plane(Normal(), d - translation.Dot(Normal()));
}

BE_INLINE Plane &Plane::TranslateSelf(const Vec3 &translation) {
    d -= translation.Dot(Normal());
    return *this;
}

BE_INLINE Plane Plane::Rotate(const Vec3 &origin, const Mat3 &axis) const {
    Plane p;
    p.Normal() = axis * Normal();
    p.d = d + origin.Dot(Normal()) - origin.Dot(p.Normal());
    return p;
}

BE_INLINE Plane &Plane::RotateSelf(const Vec3 &origin, const Mat3 &axis) {
    d += origin.Dot(Normal());
    Normal() = axis * Normal();
    d -= origin.Dot(Normal());
    return *this;
}

BE_INLINE float Plane::Distance(const Vec3 &v) const { 
    return a * v.x + b * v.y + c * v.z + d; 
}

BE_INLINE int Plane::GetSide(const Vec3 &v, const float epsilon) const {
    float d = Distance(v);
    if (d > epsilon) {
        return Side::Front;
    } else if (d < -epsilon) {
        return Side::Back;
    } else {
        return Side::On;
    }
}

BE_INLINE float Plane::RayIntersection(const Vec3 &start, const Vec3 &dir) const {
    /*float d1 = Normal().Dot(start) + d;
    if (d1 <= 0) {
        return false;
    }

    float d2 = Normal().Dot(dir);
    if (d2 >= 0) {
        return false;
    }

    t = -(d1 / d2);	
    return true;
    */
    float d1 = Normal().Dot(start) + d;
    float d2 = Normal().Dot(dir);

    if (d2 == 0.0f) {
        return FLT_MAX;
    }
    
    return -(d1 / d2);
}

BE_INLINE bool Plane::LineIntersection(const Vec3 &start, const Vec3 &end) const {
/*	float d1 = Normal().Dot(start) + d;
    if (d1 <= 0) {
        return false;
    }

    float d2 = Normal().Dot(end - start);
    if (d2 >= 0) {
        return false;
    }

    float t = -(d1 / d2);
    if (t < 0.0f || t > 1.0f) {
        return false;
    }

    return true;*/

    float d1 = Normal().Dot(start) + d;
    float d2 = Normal().Dot(end) + d;

    if (d1 == d2) {
        return false;
    }

    if (d1 > 0.0f && d2 > 0.0f) {
        return false;
    }
    
    if (d1 < 0.0f && d2 < 0.0f) {
        return false;
    }

    float fraction = (d1 / (d1 - d2));

    return (fraction >= 0.0f && fraction <= 1.0f);
}

BE_NAMESPACE_END
