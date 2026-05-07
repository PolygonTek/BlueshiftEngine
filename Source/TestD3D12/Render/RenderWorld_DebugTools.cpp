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
#include "RenderWorld.h"

void RenderWorld::DrawDebugLine(const BE1::Vec3 &start, const BE1::Vec3 &end, bool depthTest, int lifeTime) {
    /*BE1::Vec3 *v = RB_ReserveDebugPrimsVerts(Graphics::Topology::LineList, 2, debugLineColor, false, depthTest, lifeTime);
    if (v) {
        v[0] = start;
        v[1] = end;
    }*/
}

void RenderWorld::DrawDebugTriangle(const BE1::Vec3 &a, const BE1::Vec3 &b, const BE1::Vec3 &c, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugQuad(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, float size, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugHollowCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius1, const float radius2, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugArc(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugEllipse(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius1, const float radius2, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugHemisphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugHemisphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    DrawDebugCircle(origin, axis[2], radius, false, depthTest, lifeTime);
    DrawDebugArc(origin, axis[0], axis[2], radius, 0, 180, false, false, depthTest, lifeTime);
    DrawDebugArc(origin, axis[1], axis[2], radius, 0, 180, false, false, depthTest, lifeTime);
}

void RenderWorld::DrawDebugSphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugSphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    DrawDebugCircle(origin, axis[0], radius, false, depthTest, lifeTime);
    DrawDebugCircle(origin, axis[1], radius, false, depthTest, lifeTime);
    DrawDebugCircle(origin, axis[2], radius, false, depthTest, lifeTime);
}

void RenderWorld::DrawDebugAABB(const BE1::AABB &aabb, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugOBB(const BE1::OBB &obb, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugFrustum(const BE1::Frustum &frustum, bool showFromOrigin, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugCone(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool drawCap, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}

void RenderWorld::DrawDebugCone(const BE1::Vec3 &apex, const BE1::Vec3 &bottom, float radius1, float radius2, bool drawCap, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Mat3 axis;
    axis[2] = apex - bottom;
    float height = axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DrawDebugCone(bottom, axis, height, radius1, radius2, drawCap, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugConeSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool drawCap, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 apex = origin + axis[2] * height;
    BE1::Vec3 vec[2][2];
    vec[0][0] = axis[0] * radius2;
    vec[0][1] = axis[1] * radius2;
    vec[1][0] = axis[0] * radius1;
    vec[1][1] = axis[1] * radius1;

    if (radius1 == 0.0f) {
        DrawDebugCircle(apex, axis[2], radius1, twoSided, depthTest, lifeTime);
    }

    DrawDebugCircle(origin, axis[2], radius2, twoSided, depthTest, lifeTime);

    DrawDebugLine(origin + vec[0][0], apex + vec[1][0], depthTest, lifeTime);
    DrawDebugLine(origin - vec[0][0], apex - vec[1][0], depthTest, lifeTime);
    DrawDebugLine(origin + vec[0][1], apex + vec[1][1], depthTest, lifeTime);
    DrawDebugLine(origin - vec[0][1], apex - vec[1][1], depthTest, lifeTime);
}

void RenderWorld::DrawDebugCylinder(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 origin = center - axis[2] * height * 0.5f;

    DrawDebugCone(origin, axis, height, radius, radius, true, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCylinder(const BE1::Vec3 &top, const BE1::Vec3 &bottom, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    DrawDebugCone(top, bottom, radius, radius, true, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCylinderSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 top = center + axis[2] * height * 0.5f;
    BE1::Vec3 bottom = center - axis[2] * height * 0.5f;

    DrawDebugCircle(top, axis[2], radius, false, depthTest, lifeTime);
    DrawDebugCircle(bottom, -axis[2], radius, false, depthTest, lifeTime);

    BE1::Vec3 vec[2];
    vec[0] = axis[0] * radius;
    vec[1] = axis[1] * radius;

    DrawDebugLine(bottom + vec[0], top + vec[0], depthTest, lifeTime);
    DrawDebugLine(bottom - vec[0], top - vec[0], depthTest, lifeTime);
    DrawDebugLine(bottom + vec[1], top + vec[1], depthTest, lifeTime);
    DrawDebugLine(bottom - vec[1], top - vec[1], depthTest, lifeTime);
}

void RenderWorld::DrawDebugCapsule(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 a = center + axis[2] * height * 0.5f;
    BE1::Vec3 b = center - axis[2] * height * 0.5f;

    DrawDebugCone(a, axis, height, radius, radius, false, twoSided, depthTest, lifeTime);

    DrawDebugHemisphere(a, axis, radius, twoSided, depthTest, lifeTime);

    BE1::Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DrawDebugHemisphere(b, axis2, radius, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCapsule(const BE1::Vec3 &a, const BE1::Vec3 &b, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Mat3 axis;
    axis[2] = a - b;
    axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DrawDebugCone(a, b, radius, radius, false, twoSided, depthTest, lifeTime);

    DrawDebugHemisphere(a, axis, radius, twoSided, depthTest, lifeTime);

    axis[2] = -axis[2];
    axis[1] = -axis[1];
    DrawDebugHemisphere(b, axis, radius, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCapsuleSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 a = center + axis[2] * height * 0.5f;
    BE1::Vec3 b = center - axis[2] * height * 0.5f;
    BE1::Vec3 vec[2] = {
        axis[0] * radius,
        axis[1] * radius
    };

    DrawDebugLine(a + vec[0], b + vec[0], depthTest, lifeTime);
    DrawDebugLine(a - vec[0], b - vec[0], depthTest, lifeTime);
    DrawDebugLine(a + vec[1], b + vec[1], depthTest, lifeTime);
    DrawDebugLine(a - vec[1], b - vec[1], depthTest, lifeTime);

    DrawDebugHemisphereSimple(a, axis, radius, depthTest, lifeTime);

    BE1::Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DrawDebugHemisphereSimple(b, axis2, radius, depthTest, lifeTime);
}

void RenderWorld::DrawDebugArrow(const BE1::Vec3 &start, const BE1::Vec3 &end, float coneSize, float coneRadius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {

}
