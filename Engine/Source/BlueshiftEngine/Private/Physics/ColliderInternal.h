#pragma once

#include "btBulletCollisionCommon.h"

#include "BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.h"
#include "BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include "LinearMath/btConvexHullComputer.h"

#include "HACD/hacdHACD.h"

BE_NAMESPACE_BEGIN

class CollisionMaterial : public btMaterial {
public:
    int                 surfaceFlags;
};

BE_NAMESPACE_END
