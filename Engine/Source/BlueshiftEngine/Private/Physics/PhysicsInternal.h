#pragma once

#include "btBulletDynamicsCommon.h"

#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"

#include "BulletDynamics/MLCPSolvers/btDantzigSolver.h"
#include "BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h"
#include "BulletDynamics/MLCPSolvers/btMLCPSolver.h"

BE_NAMESPACE_BEGIN

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
