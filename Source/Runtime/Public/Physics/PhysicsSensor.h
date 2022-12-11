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

class btPairCachingGhostObject;

#include "Containers/Array.h"
#include "PhysicsCollidable.h"

BE_NAMESPACE_BEGIN

class PhysSensor : public PhysCollidable {
    friend class PhysicsSystem;

public:
    PhysSensor(btPairCachingGhostObject *pairCachingGhostObject, const Vec3 &centroid);
    virtual ~PhysSensor();

    virtual void                    AddToWorld(PhysicsWorld *physicsWorld) override;
    virtual void                    RemoveFromWorld() override;

    void                            GetContacts(Array<Contact> &contacts);
    void                            GetOverlaps(Array<PhysCollidable *> &collidables);

private:
    btPairCachingGhostObject *      GetPairCachingGhostObject();
    const btPairCachingGhostObject *GetPairCachingGhostObject() const;
};

BE_NAMESPACE_END
