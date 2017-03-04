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
