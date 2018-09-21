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
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN

void Collider::Purge() {
    for (int i = 0; i < collisionMeshes.Count(); i++) {
        FreeCollisionMesh(collisionMeshes[i]);
    }

    collisionMeshes.Clear();

    if (shape) {
        if (type == Collider::Type::Bvh) {
            btBvhTriangleMeshShape *bvhTriMeshShape = static_cast<btBvhTriangleMeshShape *>(shape);
            assert(bvhTriMeshShape);

            btStridingMeshInterface *meshInterface = bvhTriMeshShape->getMeshInterface();
            if (meshInterface) {
                delete meshInterface;
            }

            btTriangleInfoMap *triInfoMap = bvhTriMeshShape->getTriangleInfoMap();
            if (triInfoMap) {
                delete triInfoMap;
            }
        }

        delete shape;
        shape = nullptr;
    }
}

CollisionMesh *Collider::AllocCollisionMesh(int numVerts, int numIndexes, bool materialIndexes) const {
    CollisionMesh *collisionMesh    = new CollisionMesh;
    collisionMesh->numVerts         = numVerts;
    collisionMesh->verts            = (Vec3 *)Mem_Alloc16(numVerts * sizeof(collisionMesh->verts[0]));
    collisionMesh->numIndexes       = numIndexes;
    collisionMesh->indexes          = numIndexes > 0 ? (int *)Mem_Alloc16(numIndexes * sizeof(collisionMesh->indexes[0])) : nullptr;

    if (materialIndexes && numIndexes > 0) {
        collisionMesh->materialIndexes = (int *)Mem_Alloc16((numIndexes / 3) * sizeof(collisionMesh->materialIndexes[0]));
    } else {
        collisionMesh->materialIndexes = nullptr;
    }
    return collisionMesh;
}

void Collider::FreeCollisionMesh(CollisionMesh *cmesh) const {
    if (cmesh->verts) {
        Mem_AlignedFree(cmesh->verts);
    }
    if (cmesh->indexes) {
        Mem_AlignedFree(cmesh->indexes);
    }
    if (cmesh->materialIndexes) {
        Mem_AlignedFree(cmesh->materialIndexes);
    }

    SAFE_DELETE(cmesh);
}

void Collider::SetLocalScaling(float sx, float sy, float sz) {
    shape->setLocalScaling(btVector3(sx, sy, sz));
}

const AABB Collider::GetAABB() const {
    btVector3 aabbMin, aabbMax;
    btTransform transform;

    transform.setIdentity();
    transform.setOrigin(ToBtVector3(SystemUnitToPhysicsUnit(centroid)));

    shape->getAabb(transform, aabbMin, aabbMax);

    AABB aabb(PhysicsUnitToSystemUnit(ToVec3(aabbMin)), PhysicsUnitToSystemUnit(ToVec3(aabbMax)));
    return aabb;
}

void Collider::CreateBox(const Vec3 &center, const Vec3 &extents, float margin) {
    Purge();

    type = Type::Box;
    btVector3 halfExtents = ToBtVector3(SystemUnitToPhysicsUnit(extents));
    btBoxShape *boxShape = new btBoxShape(halfExtents);

    if (SystemUnitToPhysicsUnit(margin) < boxShape->getMargin()) {
        boxShape->setMargin(SystemUnitToPhysicsUnit(margin));
    }

    shape = boxShape;

    modelScale = Vec3::one;
    volume = extents.x * extents.y * extents.z * 8.0f;
    centroid = center;
}

void Collider::CreateSphere(const Vec3 &center, float radius) {
    Purge();
    
    type = Type::Sphere;
    btSphereShape *sphereShape = new btSphereShape(SystemUnitToPhysicsUnit(radius));
    // No need to set margin, because entire shape can be represented by margin

    shape = sphereShape;

    modelScale = Vec3::one;
    volume = (4.0f / 3.0f) * Math::Pi * radius * radius * radius;
    centroid = center;
}

void Collider::CreateCapsule(const Vec3 &center, float radius, float height) {
    Purge();
    
    type = Type::Capsule;
    btCapsuleShapeZ *capsuleShape = new btCapsuleShapeZ(
        SystemUnitToPhysicsUnit(radius), 
        SystemUnitToPhysicsUnit(height));
    // No need to set margin, because entire shape can be represented by margin

    shape = capsuleShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * (height + (radius * 4.0f / 3.0f));
    centroid = center;
}

void Collider::CreateCylinder(const Vec3 &center, float radius, float height, float margin) {
    Purge();

    type = Type::Cylinder;
    btCylinderShapeZ *cylinderShape = new btCylinderShapeZ(btVector3(
        SystemUnitToPhysicsUnit(radius + margin), 
        SystemUnitToPhysicsUnit(radius + margin), 
        SystemUnitToPhysicsUnit(height * 0.5f + margin)));

    if (SystemUnitToPhysicsUnit(margin) < cylinderShape->getMargin()) {
        cylinderShape->setMargin(SystemUnitToPhysicsUnit(margin));
    }

    shape = cylinderShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * height;
    centroid = center;
}

void Collider::CreateCone(const Vec3 &center, float radius, float height, float margin) {
    Purge();

    type = Type::Cone;
    btConeShapeZ *coneShape = new btConeShapeZ(
        SystemUnitToPhysicsUnit(radius), 
        SystemUnitToPhysicsUnit(height));

    if (SystemUnitToPhysicsUnit(margin) < coneShape->getMargin()) {
        coneShape->setMargin(SystemUnitToPhysicsUnit(margin));
    }

    shape = coneShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * height / 3.0f;
    centroid = center;
}

void Collider::CreateConvexHull(const Mesh *mesh, const Vec3 &scale, float margin) {
    Purge();

    modelScale = scale;
    volume = scale.x * scale.y * scale.z * mesh->ComputeVolume();
    centroid = scale * mesh->ComputeCentroid();

    int numPoints = 0;
    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const SubMesh *subMesh = mesh->GetSurface(i)->subMesh;
        numPoints += subMesh->NumOriginalVerts();
    }

    Array<Vec3> points;
    points.SetCount(numPoints);
    numPoints = 0;

    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const MeshSurf *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        for (int j = 0; j < subMesh->NumOriginalVerts(); j++) {
            points[numPoints++] = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[j].xyz - centroid);
        }
    }

    btConvexHullComputer *chc = new btConvexHullComputer;
    float shrink = chc->compute((const float *)points.Ptr(), sizeof(points[0]), points.Count(), SystemUnitToPhysicsUnit(margin), 1.0f);

    // convex hull have no indexes
    CollisionMesh *cmesh = AllocCollisionMesh(chc->vertices.size(), 0);
    collisionMeshes.Append(cmesh);

    for (int i = 0; i < chc->vertices.size(); i++) {
        const btVector3 &btvec3 = chc->vertices[i];
        cmesh->verts[i].Set(btvec3.x(), btvec3.y(), btvec3.z());
    }
    
    delete chc;

    btConvexHullShape *chShape = new btConvexHullShape((const btScalar *)cmesh->verts, cmesh->numVerts, sizeof(cmesh->verts[0]));
    chShape->setMargin(shrink);

    type = Type::ConvexHull;
    shape = chShape;
}

void Collider::CreateConvexDecomp(const Mesh *mesh, const Vec3 &scale, float margin) {
    Purge();

    modelScale = scale;
    volume = scale.x * scale.y * scale.z * mesh->ComputeVolume();
    centroid = scale * mesh->ComputeCentroid();

    Array<HACD::Vec3<HACD::Real> > hacdPoints;
    Array<HACD::Vec3<long> > hacdTris;
    int	indexOffset = 0;

    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const MeshSurf *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        for (int i = 0; i < subMesh->NumOriginalVerts(); i++) {
            const Vec3 &xyz = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[i].xyz - centroid);
            HACD::Vec3<HACD::Real> p(xyz.x, xyz.y, xyz.z);
            hacdPoints.Append(p);
        }

        for (int i = 0; i < subMesh->NumIndexes(); i += 3) {
            long i0 = indexOffset + subMesh->Indexes()[i];
            long i1 = indexOffset + subMesh->Indexes()[i + 1];
            long i2 = indexOffset + subMesh->Indexes()[i + 2];
            HACD::Vec3<long> tri(i0, i1, i2);
            hacdTris.Append(tri);
        }

        indexOffset += subMesh->NumOriginalVerts();
    }

    // reference: http://kmamou.blogspot.kr/2011/11/hacd-parameters.html
    HACD::HACD myHACD;
    myHACD.SetScaleFactor(MeterToUnit(20.0f)); // FIXME
    myHACD.SetPoints(hacdPoints.Ptr());
    myHACD.SetNPoints(hacdPoints.Count());
    myHACD.SetTriangles(hacdTris.Ptr());
    myHACD.SetNTriangles(hacdTris.Count());
    myHACD.SetCompacityWeight(0.1);
    myHACD.SetVolumeWeight(0.0);
    myHACD.SetNClusters(2);         // minimum number of clusters
    myHACD.SetNVerticesPerCH(100);  // max of 100 vertices per convex-hull
    myHACD.SetConcavity(100);       // maximum allowed concavity
    myHACD.SetAddExtraDistPoints(false);
    myHACD.SetAddNeighboursDistPoints(false);
    myHACD.SetAddFacesPoints(false);
    myHACD.Compute();

    size_t numClusters = myHACD.GetNClusters();

    BE_LOG(L"%i convex hulls generated\n", numClusters);

    for (int i = 0; i < numClusters; i++) {
        size_t numPoints = myHACD.GetNPointsCH(i);
        size_t numTris = myHACD.GetNTrianglesCH(i);
                
        HACD::Vec3<HACD::Real> *chPoints = new HACD::Vec3<HACD::Real>[numPoints];
        HACD::Vec3<long> *chTris = new HACD::Vec3<long>[numTris];
        myHACD.GetCH(i, chPoints, chTris);
/*
        // to shrink
        btConvexHullComputer *chc = new btConvexHullComputer;
        shrink = chc->compute((const float *)&chPoints[0], sizeof(chPoints[0]), numPoints, margin, 1.0f);

        CollisionMesh *cmesh = AllocCollisionMesh(chc->vertices.size(), 0);
        collisionMeshes.Append(cmesh);

        for (int i = 0; i < chc->vertices.size(); i++) {
            const btVector3 &btvec3 = chc->vertices[i];
            cmesh->verts[i].Set(btvec3.x(), btvec3.y(), btvec3.z());
        }

        delete chc;*/

        CollisionMesh *cmesh = AllocCollisionMesh((int)numPoints, 0);
        collisionMeshes.Append(cmesh);

        for (int i = 0; i < numPoints; i++) {
            cmesh->verts[i].Set(chPoints[i].X(), chPoints[i].Y(), chPoints[i].Z());
        }

        delete [] chPoints;
        delete [] chTris;
    }

    btCompoundShape *compoundShape = new btCompoundShape;

    for (int i = 0; i < collisionMeshes.Count(); i++) {
        const CollisionMesh *cmesh = GetCollisionMesh(i);

        btConvexHullShape *chShape = new btConvexHullShape((const btScalar *)cmesh->verts, cmesh->numVerts, sizeof(cmesh->verts[0]));
        chShape->setMargin(SystemUnitToPhysicsUnit(margin));
    
        btTransform localTransform;
        localTransform.setIdentity();
        //localTransform.setOrigin(btVector3(-centroid.x, -centroid.y, -centroid.z));
        compoundShape->addChildShape(localTransform, chShape);
    }

    type = Type::ConvexHull;
    shape = compoundShape;
}

void Collider::CreateBVH(const Mesh *mesh, bool multiMaterials, const Vec3 &scale) {
    Purge();

    modelScale = scale;
    volume = 0.0f;
    centroid.SetFromScalar(0);

    if (multiMaterials) {
        CreateBVHCMMultiMaterials(mesh, scale);
    } else {
        CreateBVHCMSingleMaterial(mesh, scale);
    }

    //btTriangleInfoMap *triangleInfoMap = new btTriangleInfoMap();
    
    //btGenerateInternalEdgeInfo(static_cast<btBvhTriangleMeshShape *>(shape), triangleInfoMap);
}

void Collider::CreateBVHCMSingleMaterial(const Mesh *mesh, const Vec3 &scale) {
    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const MeshSurf *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        CollisionMesh *cmesh = AllocCollisionMesh(subMesh->NumVerts(), subMesh->NumIndexes());
        collisionMeshes.Append(cmesh);

        for (int j = 0; j < subMesh->NumVerts(); j++) {
            cmesh->verts[j] = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[j].xyz);
        }

        // TODO: vertex hash 로 중복 vertex position 제거할 것
        for (int j = 0; j < subMesh->NumIndexes(); j++) {
            cmesh->indexes[j] = subMesh->Indexes()[j];
        }
    }

    btTriangleIndexVertexArray *indexedMeshArray = new btTriangleIndexVertexArray;

    for (int i = 0; i < collisionMeshes.Count(); i++) {
        const CollisionMesh *cmesh = GetCollisionMesh(i);

        PHY_ScalarType indexType = sizeof(cmesh->indexes[0]) == sizeof(int32_t) ? PHY_INTEGER : PHY_SHORT;

        btIndexedMesh indexedMesh;
        indexedMesh.m_numTriangles = cmesh->numIndexes / 3; // assume TRIANGLES primitive type
        indexedMesh.m_indexType = indexType;
        indexedMesh.m_triangleIndexStride = sizeof(cmesh->indexes[0]) * 3;
        indexedMesh.m_triangleIndexBase = (const byte *)cmesh->indexes;
    
        indexedMesh.m_numVertices = cmesh->numVerts;
        indexedMesh.m_vertexType = PHY_FLOAT;
        indexedMesh.m_vertexStride = sizeof(cmesh->verts[0]);
        indexedMesh.m_vertexBase = (const byte *)cmesh->verts;

        indexedMeshArray->addIndexedMesh(indexedMesh, indexType);
    }

    type = Type::Bvh;
    shape = new btBvhTriangleMeshShape(indexedMeshArray, true);
    //shape->setMargin(0.01f);
}

void Collider::CreateBVHCMMultiMaterials(const Mesh *mesh, const Vec3 &scale) {
    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const MeshSurf *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        CollisionMesh *cmesh = AllocCollisionMesh(subMesh->NumVerts(), subMesh->NumIndexes(), true);
        collisionMeshes.Append(cmesh);

        for (int j = 0; j < subMesh->NumVerts(); j++) {
            cmesh->verts[j] = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[j].xyz);
        }

        // TODO: vertex hash 로 중복 vertex position 제거할 것
        for (int j = 0; j < subMesh->NumIndexes(); j++) {
            cmesh->indexes[j] = subMesh->Indexes()[j];
        }

        for (int j = 0; j < subMesh->NumIndexes() / 3; j++) {
            cmesh->materialIndexes[j] = 0; // TODO
        }
    }

    btTriangleIndexVertexMaterialArray *indexedMeshArray = new btTriangleIndexVertexMaterialArray;

    for (int i = 0; i < collisionMeshes.Count(); i++) {
        const CollisionMesh *cmesh = GetCollisionMesh(i);

        PHY_ScalarType indexType = sizeof(cmesh->indexes[0]) == sizeof(int32_t) ? PHY_INTEGER : PHY_SHORT;

        btIndexedMesh indexedMesh;
        indexedMesh.m_numTriangles = cmesh->numIndexes / 3; // assume TRIANGLES primitive type
        indexedMesh.m_indexType = indexType;
        indexedMesh.m_triangleIndexStride = sizeof(cmesh->indexes[0]) * 3;
        indexedMesh.m_triangleIndexBase = (const byte *)cmesh->indexes;
        
        indexedMesh.m_numVertices = cmesh->numVerts;
        indexedMesh.m_vertexType = PHY_FLOAT;
        indexedMesh.m_vertexStride = sizeof(cmesh->verts[0]);
        indexedMesh.m_vertexBase = (const byte *)cmesh->verts;

        indexedMeshArray->addIndexedMesh(indexedMesh, indexType);

        btMaterialProperties mat;
        mat.m_numMaterials = colliderManager.materials.Count();
        mat.m_materialBase = (const byte *)colliderManager.materials.Ptr();
        mat.m_materialStride = sizeof(CollisionMaterial);
        mat.m_materialType = PHY_FLOAT;
        mat.m_numTriangles = cmesh->numIndexes / 3;
        mat.m_triangleMaterialsBase = (const byte *)cmesh->materialIndexes;
        mat.m_triangleMaterialStride = sizeof(cmesh->materialIndexes[0]);
        
        indexedMeshArray->addMaterialProperties(mat, PHY_INTEGER);
    }

    type = Type::Bvh;
    shape = new btMultimaterialTriangleMeshShape(indexedMeshArray, true);
    //shape->setMargin(0.001f);
}

bool Collider::Load(const char *filename, bool convexHull, const Vec3 &scale) {
    Purge();

    Mesh *mesh = meshManager.GetMesh(filename);

    if (convexHull) {
        CreateConvexHull(mesh, scale, CentiToUnit(1.0f));
        //CreateConvexDecomp(mesh, scale);
    } else {
        CreateBVH(mesh, false, scale);
    }

    meshManager.ReleaseMesh(mesh);
    return true;
}

bool Collider::Reload() {
    Str _name = name;

    // remove mangled name <CH>
    int end =_name.Find('<');
    if (end != -1) {
        _name = _name.Left(end);
    }

    bool ret = Load(_name, type == Type::ConvexHull ? true : false, modelScale);

    return ret;
}

void Collider::Write(const char *filename) {
    // TODO: Implement this !
}

BE_NAMESPACE_END
