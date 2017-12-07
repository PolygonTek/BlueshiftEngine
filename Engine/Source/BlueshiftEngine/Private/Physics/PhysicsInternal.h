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

#include "btBulletDynamicsCommon.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"

#include "BulletDynamics/MLCPSolvers/btDantzigSolver.h"
#include "BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h"
#include "BulletDynamics/MLCPSolvers/btMLCPSolver.h"

BE_NAMESPACE_BEGIN

BE_INLINE btVector3 ToBtVector3(const Vec3 &vector) {
    return btVector3(vector.x, vector.y, vector.z);
}

BE_INLINE btQuaternion ToBtQuaternion(const Quat &quaternion) {
    return btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
}

BE_INLINE Vec3 ToVec3(const btVector3 &vector) {
    return Vec3(vector.x(), vector.y(), vector.z());
}

BE_INLINE Quat ToQuat(const btQuaternion &quaternion) {
    return Quat(quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z());
}

class PhysDebugDraw : public btIDebugDraw {
public:
    PhysDebugDraw() { debugMode = 0; }
    virtual ~PhysDebugDraw();

    virtual void            setDebugMode(int debugMode);
    virtual int             getDebugMode() const { return debugMode; }

    virtual void            drawLine(const btVector3 &from, const btVector3 &to,const btVector3 &fromColor, const btVector3 &toColor);
    virtual void            drawLine(const btVector3 &from, const btVector3 &to,const btVector3 &color);
    virtual void            drawSphere(btScalar radius, const btTransform &transform, const btVector3 &color);
    virtual void            drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color);
    virtual void            drawCapsule(btScalar radius, btScalar halfHeight, int upAxis, const btTransform &transform, const btVector3 &color);
    virtual void            drawCone(btScalar radius, btScalar height, int upAxis, const btTransform &transform, const btVector3 &color);
    virtual void            drawTriangle(const btVector3 &a,const btVector3 &b, const btVector3 &c, const btVector3 &color, btScalar alpha);
    virtual void            drawContactPoint(const btVector3 &pointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color);
    virtual void            reportErrorWarning(const char *warningString);
    virtual void            draw3dText(const btVector3 &location, const char *textString);

    int                     debugMode;
};

extern PhysDebugDraw        physicsDebugDraw;

BE_NAMESPACE_END

#include "PhysicsCVars.h"
