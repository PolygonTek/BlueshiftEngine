#pragma once

/*
-------------------------------------------------------------------------------

    Ray

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Infinite straight line in three-dimensional space.
class BE_API Ray {
public:
    /// The default constructor does not initialize any members of this class.
    Ray() {}
    Ray(const Vec3 &origin, const Vec3 &direction);

    const Vec3          GetOrigin() const { return origin; }
    const Vec3          GetDirection() const { return direction; }
    const Vec3          GetDistancePoint(float distance) const;

    void                Set(const Vec3 &origin, const Vec3 &direction);

    Vec3                origin;
    Vec3                direction;
};

BE_INLINE Ray::Ray(const Vec3 &origin, const Vec3 &direction) {
    this->origin = origin;
    this->direction = direction;
}

BE_INLINE void Ray::Set(const Vec3 &origin, const Vec3 &direction) {
    this->origin = origin;
    this->direction = direction;
}

BE_INLINE const Vec3 Ray::GetDistancePoint(float distance) const {
    return origin + direction * distance;
}

BE_NAMESPACE_END
