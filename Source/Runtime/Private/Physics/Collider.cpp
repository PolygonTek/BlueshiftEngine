// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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
    for (CollisionMesh *collisionMesh : collisionMeshes) {
        FreeCollisionMesh(collisionMesh);
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
    CollisionMesh *collisionMesh    = BE_NEW CollisionMesh;
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

void Collider::FreeCollisionMesh(CollisionMesh *collisionMesh) const {
    if (collisionMesh->verts) {
        Mem_AlignedFree(collisionMesh->verts);
    }
    if (collisionMesh->indexes) {
        Mem_AlignedFree(collisionMesh->indexes);
    }
    if (collisionMesh->materialIndexes) {
        Mem_AlignedFree(collisionMesh->materialIndexes);
    }

    SAFE_DELETE(collisionMesh);
}

void Collider::SetLocalScaling(float sx, float sy, float sz) {
    shape->setLocalScaling(btVector3(sx, sy, sz));
}

const AABB Collider::GetAABB() const {
    btVector3 aabbMin, aabbMax;
    btTransform transform;

    transform.setBasis(btMatrix3x3::getIdentity());
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

void Collider::CreateCone(const Vec3 &center, float radius, float height, Axis::Enum axis, float margin) {
    Purge();

    type = Type::Cone;
    btConeShape *coneShape = nullptr;
    if (axis == Axis::X) {
        coneShape = new btConeShapeX(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    } else if (axis == Axis::Y) {
        coneShape = new btConeShape(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    } else if (axis == Axis::Z) {
        coneShape = new btConeShapeZ(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    }

    if (SystemUnitToPhysicsUnit(margin) < coneShape->getMargin()) {
        coneShape->setMargin(SystemUnitToPhysicsUnit(margin));
    }

    shape = coneShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * height / 3.0f;
    centroid = center;
}

void Collider::CreateCylinder(const Vec3 &center, float radius, float height, Axis::Enum axis, float margin) {
    Purge();

    type = Type::Cylinder;
    btCylinderShape *cylinderShape;
    if (axis == Axis::X) {
        cylinderShape = new btCylinderShapeX(btVector3(
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(height * 0.5f + margin)));
    } else if (axis == Axis::Y) {
        cylinderShape = new btCylinderShape(btVector3(
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(height * 0.5f + margin)));
    } else if (axis == Axis::Z) {
        cylinderShape = new btCylinderShapeZ(btVector3(
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(radius + margin),
            SystemUnitToPhysicsUnit(height * 0.5f + margin)));
    }

    if (SystemUnitToPhysicsUnit(margin) < cylinderShape->getMargin()) {
        cylinderShape->setMargin(SystemUnitToPhysicsUnit(margin));
    }

    shape = cylinderShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * height;
    centroid = center;
}

void Collider::CreateCapsule(const Vec3 &center, float radius, float height, Axis::Enum axis) {
    Purge();

    type = Type::Capsule;
    btCapsuleShape *capsuleShape;
    if (axis == Axis::X) {
        capsuleShape = new btCapsuleShapeX(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    } else if (axis == Axis::Y) {
        capsuleShape = new btCapsuleShape(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    } else if (axis == Axis::Z) {
        capsuleShape = new btCapsuleShapeZ(
            SystemUnitToPhysicsUnit(radius),
            SystemUnitToPhysicsUnit(height));
    }
    // No need to set margin, because entire shape can be represented by margin.

    shape = capsuleShape;

    modelScale = Vec3::one;
    volume = Math::Pi * radius * radius * (height + (radius * 4.0f / 3.0f));
    centroid = center;
}

void Collider::CreateConvexHull(const Mesh *mesh, const Vec3 &scale, float margin) {
    Purge();

    this->modelScale = scale;
    this->volume = mesh->CalculateVolumeAndCentroid(this->centroid);
    this->volume *= scale.x * scale.y * scale.z;
    this->centroid *= scale;

    int numPoints = 0;
    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const SubMesh *subMesh = mesh->GetSurface(surfaceIndex)->subMesh;
        numPoints += subMesh->NumOriginalVerts();
    }

    if (numPoints < 3) {
        BE_ERRLOG("Failed to create convex collider due to too few vertices");
        return;
    }

    // Collect all vertices to be input to compute convex hull points.
    Array<Vec3> inputVertices;
    inputVertices.SetCount(numPoints);
    Vec3 *inputVertexPtr = (Vec3 *)inputVertices.Ptr();

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const Mesh::Surface *surf = mesh->GetSurface(surfaceIndex);
        const SubMesh *subMesh = surf->subMesh;
        const VertexGenericLit *verts = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumOriginalVerts(); vertexIndex++) {
            Vec3 positionRelativeToCentroid = scale * verts[vertexIndex].xyz - centroid;
            *inputVertexPtr++ = SystemUnitToPhysicsUnit(positionRelativeToCentroid);
        }
    }

    btConvexHullComputer *convexHullComputer = new btConvexHullComputer;
    float shrunken = convexHullComputer->compute((const float *)inputVertices.Ptr(), sizeof(inputVertices[0]), inputVertices.Count(), SystemUnitToPhysicsUnit(margin), 1.0f);

    CollisionMesh *collisionMesh = AllocCollisionMesh(convexHullComputer->vertices.size(), 0);
    this->collisionMeshes.Append(collisionMesh);

    for (int i = 0; i < convexHullComputer->vertices.size(); i++) {
        const btVector3 &btvec3 = convexHullComputer->vertices[i];
        collisionMesh->verts[i].Set(btvec3.x(), btvec3.y(), btvec3.z());
    }
    
    delete convexHullComputer;

    btConvexHullShape *convexHullShape = new btConvexHullShape((const btScalar *)collisionMesh->verts, collisionMesh->numVerts, sizeof(collisionMesh->verts[0]));
    convexHullShape->setMargin(shrunken);
    convexHullShape->initializePolyhedralFeatures(0);

    this->type = Type::ConvexHull;
    this->shape = convexHullShape;
}

void Collider::CreateConvexDecomp(const Mesh *mesh, const Vec3 &scale, float margin) {
    Purge();

    this->modelScale = scale;
    this->volume = mesh->CalculateVolumeAndCentroid(this->centroid);
    this->volume *= scale.x * scale.y * scale.z;
    this->centroid *= scale;

    Array<HACD::Vec3<HACD::Real> > hacdPoints;
    Array<HACD::Vec3<long> > hacdTris;
    int	indexOffset = 0;

    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const Mesh::Surface *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        for (int i = 0; i < subMesh->NumOriginalVerts(); i++) {
            const Vec3 &xyz = SystemUnitToPhysicsUnit(scale * subMesh->verts[i].xyz - centroid);
            HACD::Vec3<HACD::Real> p(xyz.x, xyz.y, xyz.z);
            hacdPoints.Append(p);
        }

        for (int i = 0; i < subMesh->NumIndexes(); i += 3) {
            long i0 = indexOffset + subMesh->indexes[i];
            long i1 = indexOffset + subMesh->indexes[i + 1];
            long i2 = indexOffset + subMesh->indexes[i + 2];
            HACD::Vec3<long> tri(i0, i1, i2);
            hacdTris.Append(tri);
        }

        indexOffset += subMesh->NumOriginalVerts();
    }

    if (hacdPoints.Count() < 3) {
        BE_ERRLOG("Failed to cerate convex collider due to too few vertices");
        return;
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

    BE_LOG("%i convex hulls generated\n", numClusters);

    for (int i = 0; i < numClusters; i++) {
        size_t numPoints = myHACD.GetNPointsCH(i);
        size_t numTris = myHACD.GetNTrianglesCH(i);
                
        HACD::Vec3<HACD::Real> *chPoints = BE_NEW HACD::Vec3<HACD::Real>[numPoints];
        HACD::Vec3<long> *chTris = BE_NEW HACD::Vec3<long>[numTris];
        myHACD.GetCH(i, chPoints, chTris);
/*
        // to shrink
        btConvexHullComputer *chc = new btConvexHullComputer;
        shrink = chc->compute((const float *)&chPoints[0], sizeof(chPoints[0]), numPoints, margin, 1.0f);

        CollisionMesh *collisionMesh = AllocCollisionMesh(chc->vertices.size(), 0);
        collisionMeshes.Append(collisionMesh);

        for (int i = 0; i < chc->vertices.size(); i++) {
            const btVector3 &btvec3 = chc->vertices[i];
            collisionMesh->verts[i].Set(btvec3.x(), btvec3.y(), btvec3.z());
        }

        delete chc;*/

        CollisionMesh *collisionMesh = AllocCollisionMesh((int)numPoints, 0);
        collisionMeshes.Append(collisionMesh);

        for (int i = 0; i < numPoints; i++) {
            collisionMesh->verts[i].Set(chPoints[i].X(), chPoints[i].Y(), chPoints[i].Z());
        }

        delete [] chPoints;
        delete [] chTris;
    }

    btCompoundShape *compoundShape = new btCompoundShape;

    for (const CollisionMesh *collisionMesh : collisionMeshes) {
        btConvexHullShape *convexHullShape = new btConvexHullShape((const btScalar *)collisionMesh->verts, collisionMesh->numVerts, sizeof(collisionMesh->verts[0]));
        convexHullShape->setMargin(SystemUnitToPhysicsUnit(margin));
    
        btTransform localTransform;
        localTransform.setIdentity();
        //localTransform.setOrigin(btVector3(-centroid.x, -centroid.y, -centroid.z));
        compoundShape->addChildShape(localTransform, convexHullShape);
    }

    this->type = Type::ConvexHull;
    this->shape = compoundShape;
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
        const Mesh::Surface *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        CollisionMesh *collisionMesh = AllocCollisionMesh(subMesh->NumVerts(), subMesh->NumIndexes());
        collisionMeshes.Append(collisionMesh);

        for (int j = 0; j < subMesh->NumVerts(); j++) {
            collisionMesh->verts[j] = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[j].xyz);
        }

        // TODO: Remove duplicated vertex position using vertex hash.
        for (int j = 0; j < subMesh->NumIndexes(); j++) {
            collisionMesh->indexes[j] = subMesh->Indexes()[j];
        }
    }

    btTriangleIndexVertexArray *indexedMeshArray = new btTriangleIndexVertexArray;

    for (const CollisionMesh *collisionMesh : collisionMeshes) {
        PHY_ScalarType indexType = sizeof(collisionMesh->indexes[0]) == sizeof(int32_t) ? PHY_INTEGER : PHY_SHORT;

        btIndexedMesh indexedMesh;
        indexedMesh.m_numTriangles = collisionMesh->numIndexes / 3; // assume TRIANGLES primitive type
        indexedMesh.m_indexType = indexType;
        indexedMesh.m_triangleIndexStride = sizeof(collisionMesh->indexes[0]) * 3;
        indexedMesh.m_triangleIndexBase = (const byte *)collisionMesh->indexes;
    
        indexedMesh.m_numVertices = collisionMesh->numVerts;
        indexedMesh.m_vertexType = PHY_FLOAT;
        indexedMesh.m_vertexStride = sizeof(collisionMesh->verts[0]);
        indexedMesh.m_vertexBase = (const byte *)collisionMesh->verts;

        indexedMeshArray->addIndexedMesh(indexedMesh, indexType);
    }

    type = Type::Bvh;
    shape = new btBvhTriangleMeshShape(indexedMeshArray, true);
    //shape->setMargin(0.01f);
}

void Collider::CreateBVHCMMultiMaterials(const Mesh *mesh, const Vec3 &scale) {
    for (int i = 0; i < mesh->NumSurfaces(); i++) {
        const Mesh::Surface *surf = mesh->GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        CollisionMesh *collisionMesh = AllocCollisionMesh(subMesh->NumVerts(), subMesh->NumIndexes(), true);
        collisionMeshes.Append(collisionMesh);

        for (int j = 0; j < subMesh->NumVerts(); j++) {
            collisionMesh->verts[j] = SystemUnitToPhysicsUnit(scale * subMesh->Verts()[j].xyz);
        }

        // TODO: Remove duplicated vertex position using vertex hash.
        for (int j = 0; j < subMesh->NumIndexes(); j++) {
            collisionMesh->indexes[j] = subMesh->Indexes()[j];
        }

        for (int j = 0; j < subMesh->NumIndexes() / 3; j++) {
            collisionMesh->materialIndexes[j] = 0; // TODO
        }
    }

    btTriangleIndexVertexMaterialArray *indexedMeshArray = new btTriangleIndexVertexMaterialArray;

    for (const CollisionMesh *collisionMesh : collisionMeshes) {
        PHY_ScalarType indexType = sizeof(collisionMesh->indexes[0]) == sizeof(int32_t) ? PHY_INTEGER : PHY_SHORT;

        btIndexedMesh indexedMesh;
        indexedMesh.m_numTriangles = collisionMesh->numIndexes / 3; // assume TRIANGLES primitive type
        indexedMesh.m_indexType = indexType;
        indexedMesh.m_triangleIndexStride = sizeof(collisionMesh->indexes[0]) * 3;
        indexedMesh.m_triangleIndexBase = (const byte *)collisionMesh->indexes;
        
        indexedMesh.m_numVertices = collisionMesh->numVerts;
        indexedMesh.m_vertexType = PHY_FLOAT;
        indexedMesh.m_vertexStride = sizeof(collisionMesh->verts[0]);
        indexedMesh.m_vertexBase = (const byte *)collisionMesh->verts;

        indexedMeshArray->addIndexedMesh(indexedMesh, indexType);

        btMaterialProperties mat;
        mat.m_numMaterials = colliderManager.materials.Count();
        mat.m_materialBase = (const byte *)colliderManager.materials.Ptr();
        mat.m_materialStride = sizeof(CollisionMaterial);
        mat.m_materialType = PHY_FLOAT;
        mat.m_numTriangles = collisionMesh->numIndexes / 3;
        mat.m_triangleMaterialsBase = (const byte *)collisionMesh->materialIndexes;
        mat.m_triangleMaterialStride = sizeof(collisionMesh->materialIndexes[0]);
        
        indexedMeshArray->addMaterialProperties(mat, PHY_INTEGER);
    }

    type = Type::Bvh;
    shape = new btMultimaterialTriangleMeshShape(indexedMeshArray, true);
    //shape->setMargin(0.001f);
}

struct BuoyancyConvexVolume {
    Array<Plane> planes;

    bool Contains(const Vec3 &point, float tolerance) const {
        for (const Plane &plane : planes) {
            if (plane.Distance(point) > tolerance) {
                return false;
            }
        }
        return !planes.IsEmpty();
    }
};

static bool CollectBuoyancyConvexVolumes(const btCollisionShape *collisionShape, const btTransform &parentTransform, const Vec3 &colliderCentroid, Array<BuoyancyConvexVolume> &convexVolumes) {
    if (collisionShape->isCompound()) {
        const btCompoundShape *compoundShape = static_cast<const btCompoundShape *>(collisionShape);
        for (int childIndex = 0; childIndex < compoundShape->getNumChildShapes(); ++childIndex) {
            const btTransform childTransform = parentTransform * compoundShape->getChildTransform(childIndex);
            if (!CollectBuoyancyConvexVolumes(compoundShape->getChildShape(childIndex), childTransform, colliderCentroid, convexVolumes)) {
                return false;
            }
        }
        return true;
    }

    if (!collisionShape->isConvex()) {
        return false;
    }

    const btConvexShape *convexShape = static_cast<const btConvexShape *>(collisionShape);
    btShapeHull shapeHull(convexShape);
    if (!shapeHull.buildHull(convexShape->getMargin()) || shapeHull.numVertices() < 4 || shapeHull.numTriangles() < 4) {
        return false;
    }

    Array<Vec3> vertices;
    vertices.SetCount(shapeHull.numVertices());

    Vec3 hullCenter = Vec3::zero;
    const btVector3 *shapeHullVertices = shapeHull.getVertexPointer();
    for (int vertexIndex = 0; vertexIndex < shapeHull.numVertices(); ++vertexIndex) {
        const btVector3 transformedVertex = parentTransform * shapeHullVertices[vertexIndex];
        Vec3 &vertex = vertices[vertexIndex];
        vertex = PhysicsUnitToSystemUnit(ToVec3(transformedVertex)) + colliderCentroid;
        hullCenter += vertex;
    }
    hullCenter /= static_cast<float>(vertices.Count());

    BuoyancyConvexVolume &convexVolume = convexVolumes.Alloc();
    convexVolume.planes.Reserve(shapeHull.numTriangles());

    const unsigned int *indices = shapeHull.getIndexPointer();
    for (int triangleIndex = 0; triangleIndex < shapeHull.numTriangles(); ++triangleIndex) {
        const Vec3 &a = vertices[indices[triangleIndex * 3 + 0]];
        const Vec3 &b = vertices[indices[triangleIndex * 3 + 1]];
        const Vec3 &c = vertices[indices[triangleIndex * 3 + 2]];

        Vec3 normal = (b - a).Cross(c - a);
        if (normal.Normalize() == 0.0f) {
            continue;
        }

        Plane plane(normal, a);
        if (plane.Distance(hullCenter) > 0.0f) {
            plane.Flip();
        }

        convexVolume.planes.Append(plane);
    }

    if (convexVolume.planes.IsEmpty()) {
        convexVolumes.SetCount(convexVolumes.Count() - 1);
        return false;
    }
    return true;
}

static bool IsInsideAnyBuoyancyConvexVolume(const Array<BuoyancyConvexVolume> &convexVolumes, const Vec3 &point, float tolerance) {
    for (const BuoyancyConvexVolume &convexVolume : convexVolumes) {
        if (convexVolume.Contains(point, tolerance)) {
            return true;
        }
    }
    return false;
}

bool Collider::GenerateBuoyancyPontoons(const PhysBuoyancyPontoonSettings &settings, Array<PhysBuoyancyPontoon> &pontoons) const {
    pontoons.Clear();

    if (!shape) {
        return false;
    }

    const uint32_t voxelResolution = Clamp(settings.voxelResolution, 1u, 32u);
    const uint32_t subsamplesPerAxis = Clamp(settings.subsamplesPerAxis, 1u, 4u);
    if (voxelResolution != settings.voxelResolution || subsamplesPerAxis != settings.subsamplesPerAxis) {
        BE_WARNLOG("Collider::GenerateBuoyancyPontoons: clamped invalid settings (voxelResolution: %u -> %u, subsamplesPerAxis: %u -> %u)\n",
            settings.voxelResolution, voxelResolution, settings.subsamplesPerAxis, subsamplesPerAxis);
    }

    Array<BuoyancyConvexVolume> convexVolumes;
    if (!CollectBuoyancyConvexVolumes(shape, btTransform::getIdentity(), centroid, convexVolumes) || convexVolumes.IsEmpty()) {
        return false;
    }

    const AABB bounds = GetAABB();
    if (bounds.IsCleared()) {
        return false;
    }

    const Vec3 boundsSize = bounds.Sizes();
    const float longestAxisSize = boundsSize.MaxComponent();
    if (boundsSize.MinComponent() <= 0.0f || longestAxisSize <= 0.0f) {
        return false;
    }

    const float targetVoxelSize = longestAxisSize / static_cast<float>(voxelResolution);
    int voxelCounts[3];
    for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
        voxelCounts[axisIndex] = Max(1, static_cast<int>(Math::Ceil(boundsSize[axisIndex] / targetVoxelSize)));
    }

    const Vec3 voxelSize(
        boundsSize.x / static_cast<float>(voxelCounts[0]),
        boundsSize.y / static_cast<float>(voxelCounts[1]),
        boundsSize.z / static_cast<float>(voxelCounts[2]));
    const Vec3 voxelExtents = voxelSize * 0.5f;
    const float voxelVolume = voxelSize.x * voxelSize.y * voxelSize.z;
    const float containmentTolerance = longestAxisSize * 1e-5f;
    const int subsampleCount = static_cast<int>(subsamplesPerAxis * subsamplesPerAxis * subsamplesPerAxis);
    const float inverseSubsamplesPerAxis = 1.0f / static_cast<float>(subsamplesPerAxis);

    pontoons.Reserve(voxelCounts[0] * voxelCounts[1] * voxelCounts[2]);

    float generatedVolume = 0.0f;
    for (int voxelZ = 0; voxelZ < voxelCounts[2]; ++voxelZ) {
        for (int voxelY = 0; voxelY < voxelCounts[1]; ++voxelY) {
            for (int voxelX = 0; voxelX < voxelCounts[0]; ++voxelX) {
                const Vec3 voxelMins = bounds[0] + Vec3(
                    voxelSize.x * static_cast<float>(voxelX),
                    voxelSize.y * static_cast<float>(voxelY),
                    voxelSize.z * static_cast<float>(voxelZ));

                Vec3 occupiedCenterSum = Vec3::zero;
                int occupiedSubsampleCount = 0;
                for (uint32_t sampleZ = 0; sampleZ < subsamplesPerAxis; ++sampleZ) {
                    for (uint32_t sampleY = 0; sampleY < subsamplesPerAxis; ++sampleY) {
                        for (uint32_t sampleX = 0; sampleX < subsamplesPerAxis; ++sampleX) {
                            const Vec3 sampleFraction(
                                (static_cast<float>(sampleX) + 0.5f) * inverseSubsamplesPerAxis,
                                (static_cast<float>(sampleY) + 0.5f) * inverseSubsamplesPerAxis,
                                (static_cast<float>(sampleZ) + 0.5f) * inverseSubsamplesPerAxis);
                            const Vec3 samplePosition = voxelMins + voxelSize * sampleFraction;
                            if (IsInsideAnyBuoyancyConvexVolume(convexVolumes, samplePosition, containmentTolerance)) {
                                occupiedCenterSum += samplePosition;
                                occupiedSubsampleCount++;
                            }
                        }
                    }
                }

                if (occupiedSubsampleCount == 0) {
                    continue;
                }

                PhysBuoyancyPontoon &pontoon = pontoons.Alloc();
                pontoon.localCenter = occupiedCenterSum / static_cast<float>(occupiedSubsampleCount);
                pontoon.localAxis = Mat3::identity;
                pontoon.localExtents = voxelExtents;
                pontoon.volume = voxelVolume * static_cast<float>(occupiedSubsampleCount) / static_cast<float>(subsampleCount);
                generatedVolume += pontoon.volume;
            }
        }
    }

    if (pontoons.IsEmpty() || generatedVolume <= 0.0f) {
        pontoons.Clear();
        return false;
    }

    if (volume > 0.0f) {
        const float volumeScale = volume / generatedVolume;
        for (PhysBuoyancyPontoon &pontoon : pontoons) {
            pontoon.volume *= volumeScale;
        }
    }
    return true;
}

bool Collider::Load(const char *filename, bool convexHull, const Vec3 &scale) {
    Purge();

    Mesh *mesh = meshManager.GetMesh(filename);

    if (convexHull) {
        CreateConvexHull(mesh, scale, CmToUnit(0.0f));
        //CreateConvexDecomp(mesh, scale);
    } else {
        CreateBVH(mesh, false, scale);
    }

    meshManager.ReleaseMesh(mesh);
    return true;
}

bool Collider::Reload() {
    if (unnamedIndex >= 0) {
        return false;
    }

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
