#pragma once

BE_NAMESPACE_BEGIN

class Vec3;
class PhysCollidable;

class PhysCollisionPair {
public:
    PhysCollisionPair() {}
    PhysCollisionPair(const PhysCollidable *a, const PhysCollidable *b);

                            operator int() const { return PhysCollisionPair::Hash(*this); }
    bool                    operator<(const PhysCollisionPair &other) const { return Compare(other) == -1 ? true : false; }
    bool                    operator>(const PhysCollisionPair &other) const { return Compare(other) == 1 ? true : false; }

    int                     Compare(const PhysCollisionPair &other) const;

    static int              Hash(const PhysCollisionPair &pair);

    const PhysCollidable *  a;
    const PhysCollidable *  b;
};

class PhysCollisionListener {
public:
    PhysCollisionListener() {}
    virtual ~PhysCollisionListener() {};

    virtual void            Collide(const PhysCollidable *objectA, const PhysCollidable *objectB, const Vec3 &point, const Vec3 &normal, float distance, float impulse) = 0;
};

BE_NAMESPACE_END
