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
#include "Render/Render.h"
#include "Physics/Physics.h"
#include "Physics/Collider.h"
#include "ColliderInternal.h"

BE_NAMESPACE_BEGIN
    
ColliderManager	colliderManager;

void ColliderManager::Init() {
    colliderHashMap.Init(1024, 64, 64);

    // TODO
    CollisionMaterial *cmat = new CollisionMaterial;
    cmat->surfaceFlags = 1;
    materials.Append(cmat);
}

void ColliderManager::Shutdown() {
    materials.DeleteContents(true);
    
    for (int i = 0; i < colliderHashMap.Count(); i++) {
        const auto *entry = colliderManager.colliderHashMap.GetByIndex(i);
        Collider *collider = entry->second;        
        collider->Purge();
        delete collider;
    }
    
    colliderHashMap.Clear();
    
    for (int i = 0; i < unnamedColliders.Count(); i++) {
        Collider *collider = colliderManager.unnamedColliders[i];
        if (collider) {
            delete collider;
        }
    }

    unnamedColliders.Clear();
}

Collider *ColliderManager::AllocUnnamedCollider() {
    Collider *collider = new Collider;
    collider->refCount = 1;

    int index = unnamedColliders.FindNull();
    if (index == -1) {
        index = unnamedColliders.Append(collider);
    } else {
        unnamedColliders[index] = collider;
    }

    collider->unnamedIndex = index;

    return collider;
}

Collider *ColliderManager::AllocCollider(const char *name) {
    if (colliderHashMap.Get(name)) {
        BE_FATALERROR(L"%hs collider already allocated", name);
    }
    
    Collider *collider = new Collider;
    collider->name = name;
    collider->refCount = 1;
    colliderHashMap.Set(collider->name, collider);

    return collider;
}

void ColliderManager::DestroyCollider(Collider *collider) {
    if (collider->refCount > 1) {
        BE_WARNLOG(L"ColliderManager::DestroyCollider: collider '%hs' has %i reference count\n", collider->name.c_str(), collider->refCount);
    }

    if (collider->name[0]) {
        colliderHashMap.Remove(collider->name);
        delete collider;
    } else {
        int index = collider->unnamedIndex;
        delete unnamedColliders[index];
        unnamedColliders[index] = nullptr;
    }
}

void ColliderManager::ReleaseCollider(Collider *collider, bool immediateDestroy) {
    if (collider->refCount > 0) {
        collider->refCount--;
    }

    if (immediateDestroy && collider->refCount == 0) {
        DestroyCollider(collider);
    }
}

void ColliderManager::DestroyUnusedColliders() {
    Array<Collider *> removeArray;

    for (int i = 0; i < colliderHashMap.Count(); i++) {
        const auto *entry = colliderHashMap.GetByIndex(i);
        Collider *collider = entry->second;

        if (collider && collider->refCount == 0) {
            removeArray.Append(collider);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyCollider(removeArray[i]);
    }
}

static Str MangleName(const char *name, const Vec3 &scale, bool convexHull) {
    Str _name = name;
    _name += va("<scale='%.4f %.4f %.4f' convex=%i", scale.x, scale.y, scale.z, convexHull ? 1 : 0);	
    return _name;
}

Collider *ColliderManager::FindCollider(const char *name, const Vec3 &scale, bool convexHull) const {
    const auto *entry = colliderHashMap.Get(MangleName(name, scale, convexHull));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Collider *ColliderManager::GetCollider(const char *name, const Vec3 &scale, bool convexHull) {
    if (!name || name[0] == 0) {
        return nullptr;
    }

    Collider *collider = FindCollider(name, scale, convexHull);
    if (collider) {
        collider->refCount++;
        return collider;
    }

    collider = AllocCollider(MangleName(name, scale, convexHull));
    if (!collider->Load(name, convexHull, scale)) {
        DestroyCollider(collider);
        BE_WARNLOG(L"Couldn't load collider \"%hs\"\n", name);
        return nullptr;
    }

    return collider;
}

BE_NAMESPACE_END
