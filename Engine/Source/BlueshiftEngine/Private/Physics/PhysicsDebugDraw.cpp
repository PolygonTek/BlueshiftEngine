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
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN

PhysDebugDraw	physicsDebugDraw;

BE_INLINE const Vec3 btVector3ToVec3(const btVector3 &btVec) {
    return Vec3(btVec.x(), btVec.y(), btVec.z());
}

BE_INLINE const Mat3 btMatrix3x3ToMat3(const btMatrix3x3 &btMat) {
    return Mat3(btMat[0][0], btMat[1][0], btMat[2][0],
        btMat[0][1], btMat[1][1], btMat[2][1],
        btMat[0][2], btMat[1][2], btMat[2][2]);
}

PhysDebugDraw::~PhysDebugDraw() {
}

void PhysDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor) {
    Vec3 start = btVector3ToVec3(from);
    Vec3 end = btVector3ToVec3(to);
    Color4 _color = Color4(btVector3ToVec3(fromColor).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugLine(start, end, 1, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    Vec3 start = btVector3ToVec3(from);
    Vec3 end = btVector3ToVec3(to);
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugLine(start, end, 1, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawSphere(btScalar radius, const btTransform &transform, const btVector3 &color) {
    Vec3 origin = btVector3ToVec3(transform.getOrigin());
    Mat3 axis = btMatrix3x3ToMat3(transform.getBasis());
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugSphereSimple(origin, axis, radius, 1, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color) {
    drawSphere(radius, btTransform(btMatrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1)), color);
}

void PhysDebugDraw::drawCapsule(btScalar radius, btScalar halfHeight, int upAxis, const btTransform &transform, const btVector3 &color) {
    Vec3 origin = btVector3ToVec3(transform.getOrigin());
    Mat3 axis = btMatrix3x3ToMat3(transform.getBasis());
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugCapsuleSimple(origin, axis, halfHeight * 2.0f, radius, 1, true, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawCone(btScalar radius, btScalar height, int upAxis, const btTransform &transform, const btVector3 &color) {
    Vec3 origin = btVector3ToVec3(transform.getOrigin());
    Mat3 axis = btMatrix3x3ToMat3(transform.getBasis());
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugConeSimple(origin, axis, height, 0, radius, true, 1, true, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawTriangle(const btVector3 &a, const btVector3 &b, const btVector3 &c, const btVector3 &color, btScalar alpha) {
    Vec3 _a = btVector3ToVec3(a);
    Vec3 _b = btVector3ToVec3(b);
    Vec3 _c = btVector3ToVec3(c);
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), alpha);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugTriangle(_a, _b, _c, 1, true, physics_showWireframe.GetInteger() == 2 ? false : true);
}

void PhysDebugDraw::drawContactPoint(const btVector3 &pointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {
    Vec3 p = btVector3ToVec3(pointOnB);
    Vec3 n = btVector3ToVec3(normalOnB);
    Color4 _color = Color4(btVector3ToVec3(color).ToColor3(), 1.0f);

    renderSystem.GetRenderWorld()->SetDebugColor(_color, Color4::zero);
    renderSystem.GetRenderWorld()->DebugLine(p, p + n, 2, physics_showWireframe.GetInteger() == 2 ? false : true, lifeTime);
}

void PhysDebugDraw::reportErrorWarning(const char *warningString) {
    BE_WARNLOG(L"PhysDebugDraw::reportErrorWarning: %hs\n", warningString);
}

void PhysDebugDraw::draw3dText(const btVector3 &location, const char *textString) {
    //Vec3 p = btVector3ToVec3(location);
}

void PhysDebugDraw::setDebugMode(int debugMode) {
    this->debugMode = debugMode;
}

BE_NAMESPACE_END
