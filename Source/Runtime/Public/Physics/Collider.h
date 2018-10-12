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
#include "Containers/Array.h"
#include "Containers/HashMap.h"

class btCollisionShape;

BE_NAMESPACE_BEGIN

class Mesh;
class CollisionMaterial;

class CollisionMesh {
    friend class Collider;

public:
    int                         numVerts;
    int                         numIndexes;

    Vec3 *                      verts;
    int *                       indexes;

    int	*                       materialIndexes;
};

class Collider {
    friend class ColliderManager;
    friend class PhysicsSystem;
    friend class PhysicsWorld;

public:
    enum Type {
        Box,
        Sphere,
        Capsule,
        Cylinder,
        Cone,
        MultiSphere,
        ConvexHull,
        Bvh
    };

    Collider();
    ~Collider();

                                /// Returns center of mass position.
    const Vec3 &                GetCentroid() const { return centroid; }

    void                        Purge();

                                /// Create box shaped collider with the given parameters.
    void                        CreateBox(const Vec3 &center, const Vec3 &extents, float margin = CentiToUnit(0.1f));

                                /// Create cylinder shaped collider with the given parameters.
    void                        CreateCylinder(const Vec3 &center, float radius, float height, float margin = CentiToUnit(0.1f));

                                /// Create cone shaped collider with the given parameters.
    void                        CreateCone(const Vec3 &center, float radius, float height, float margin = CentiToUnit(0.1f));

                                /// Create sphere shaped collider with the given parameters.
    void                        CreateSphere(const Vec3 &center, float radius);

                                /// Create capsule shaped collider with the given parameters.
    void                        CreateCapsule(const Vec3 &center, float radius, float height);

    void                        SetLocalScaling(float sx, float sy, float sz);

                                /// Returns axis-aligned bounding box in system units
    const AABB                  GetAABB() const;

                                /// Returns volume in system units
    float                       GetVolume() const { return volume; }

    bool                        Load(const char *filename, bool convexHull, const Vec3 &scale);
    bool                        Reload();
    void                        Write(const char *filename);

    const Collider *            AddRefCount() const { refCount++; return this; }

private:
    int                         NumCollisionMeshes() const { return collisionMeshes.Count(); }
    CollisionMesh *             GetCollisionMesh(int index) const { assert(index >= 0 && index < collisionMeshes.Count()); return collisionMeshes[index]; }
    CollisionMesh *             AllocCollisionMesh(int numVerts, int numIndexes, bool materialIndexes = false) const;
    void                        CreateConvexHull(const Mesh *mesh, const Vec3 &scale = Vec3::one, float margin = CentiToUnit(0.1f));
    void                        CreateConvexDecomp(const Mesh *mesh, const Vec3 &scale = Vec3::one, float margin = CentiToUnit(0.1f));
    void                        CreateBVH(const Mesh *mesh, bool multiMaterials = false, const Vec3 &scale = Vec3::one);
    void                        CreateBVHCMSingleMaterial(const Mesh *mesh, const Vec3 &scale = Vec3::one);
    void                        CreateBVHCMMultiMaterials(const Mesh *mesh, const Vec3 &scale = Vec3::one);
    void                        FreeCollisionMesh(CollisionMesh *mesh) const;
    
    Str                         name;
    mutable int                 refCount;
    int                         unnamedIndex;

    Type                        type;
    Vec3                        centroid;           ///< Position of center of mass in system units
    float                       volume;             ///< Volume in system units
    Vec3                        modelScale;
    btCollisionShape *          shape;
    Array<CollisionMesh *>      collisionMeshes;
};

BE_INLINE Collider::Collider() {
    refCount                    = 0;
    unnamedIndex                = -1;
    shape                       = nullptr;
}

BE_INLINE Collider::~Collider() {
    Purge();
}

class ColliderManager {
    friend class Collider;

public:
    void                        Init();
    void                        Shutdown();
    
    Collider *                  AllocCollider(const char *name);
    Collider *                  AllocUnnamedCollider();
    Collider *                  FindCollider(const char *name, const Vec3 &scale, bool convexHull) const;
    Collider *                  GetCollider(const char *name, const Vec3 &scale, bool convexHull);

    void                        ReleaseCollider(Collider *collider, bool immediateDestroy = false);
    void                        DestroyCollider(Collider *collider);
    void                        DestroyUnusedColliders();

    static Collider *           defaultCollider;

private:

    Array<CollisionMaterial *>  materials;

    StrIHashMap<Collider *>     colliderHashMap;
    Array<Collider *>           unnamedColliders;
};

extern ColliderManager          colliderManager;

BE_NAMESPACE_END
