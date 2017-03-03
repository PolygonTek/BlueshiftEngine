#pragma once

class btPairCachingGhostObject;

#include "Containers/Array.h"
#include "PhysicsCollidable.h"

BE_NAMESPACE_BEGIN

class Vec3;

class PhysSensor : public PhysCollidable {
    friend class PhysicsSystem;

public:
    PhysSensor(btPairCachingGhostObject *pairCachingGhostObject, const Vec3 &centroid);
    virtual ~PhysSensor();

    void                            GetContacts(Array<Contact> &contacts);
    void                            GetOverlaps(Array<PhysCollidable *> &collidables);

private:
    btPairCachingGhostObject *      GetPairCachingGhostObject();
    const btPairCachingGhostObject *GetPairCachingGhostObject() const;
};

BE_NAMESPACE_END
