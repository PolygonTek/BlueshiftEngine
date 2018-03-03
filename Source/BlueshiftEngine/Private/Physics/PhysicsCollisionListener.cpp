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
#include "Physics/Physics.h"

BE_NAMESPACE_BEGIN

PhysCollisionPair::PhysCollisionPair(const PhysCollidable *a, const PhysCollidable *b) {
    this->a = a;
    this->b = b;
}

int PhysCollisionPair::Compare(const PhysCollisionPair &other) const {
    intptr_t min_addr1, max_addr1;
    intptr_t min_addr2, max_addr2;

    if ((intptr_t)(a) < (intptr_t)(b)) {
        min_addr1 = (intptr_t)(a);
        max_addr1 = (intptr_t)(b);
    } else {
        min_addr1 = (intptr_t)(b);
        max_addr1 = (intptr_t)(a);
    }

    if ((intptr_t)(other.a) < (intptr_t)(other.b)) {
        min_addr2 = (intptr_t)(other.a);
        max_addr2 = (intptr_t)(other.b);
    } else {
        min_addr2 = (intptr_t)(other.b);
        max_addr2 = (intptr_t)(other.a);
    }

    if (min_addr1 < min_addr2) {
        return -1;
    } else if (min_addr1 > min_addr2) {
        return 1;
    }

    if (max_addr1 < max_addr2) {
        return -1;
    } else if (max_addr1 > max_addr2) {
        return 1;
    }

    return 0;
}

int PhysCollisionPair::Hash(const PhysCollisionPair &pair) {
    return (int)((intptr_t)(pair.a) + (intptr_t)(pair.b));
}

BE_NAMESPACE_END
