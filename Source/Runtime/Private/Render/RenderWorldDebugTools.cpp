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
#include "RenderInternal.h"
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

void RenderWorld::ClearDebugPrimitives(int time) {
    RB_ClearDebugPrimitives(time);
}

void RenderWorld::DebugLine(const Vec3 &start, const Vec3 &end, float lineWidth, bool depthTest, int lifeTime) {
    if (lineWidth > 0 && debugLineColor.a > 0) {
        Vec3 *v = RB_ReserveDebugPrimsVerts(RHI::Topology::LineList, 2, debugLineColor, lineWidth, false, depthTest, lifeTime);
        if (v) {
            v[0] = start;
            v[1] = end;
        }
    }
}

void RenderWorld::DebugTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    if (debugFillColor.a > 0) {
        Vec3 *v = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleList, 3, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (v) {
            v[0] = a;
            v[1] = b;
            v[2] = c;
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        DebugLine(a, b, lineWidth, depthTest, lifeTime);
        DebugLine(b, c, lineWidth, depthTest, lifeTime);
        DebugLine(c, a, lineWidth, depthTest, lifeTime);
    }
}

void RenderWorld::DebugQuad(const Vec3 &origin, const Vec3 &right, const Vec3 &up, float size, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 sr = right * size;
    Vec3 su = up * size;

    Vec3 v[4];
    v[0] = origin + sr - su;
    v[1] = origin - sr - su;
    v[2] = origin - sr + su;
    v[3] = origin + sr + su;

    if (debugFillColor.a > 0) {
        Vec3 *fv = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, 4, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fv) {
            fv[0] = v[0];
            fv[1] = v[1];
            fv[2] = v[2];
            fv[3] = v[3];
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        DebugLine(v[0], v[1], lineWidth, depthTest, lifeTime);
        DebugLine(v[1], v[2], lineWidth, depthTest, lifeTime);
        DebugLine(v[2], v[3], lineWidth, depthTest, lifeTime);
        DebugLine(v[3], v[0], lineWidth, depthTest, lifeTime);
    }
}

void RenderWorld::DebugCircle(const Vec3 &origin, const Vec3 &dir, const float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    const int numSteps = 48;
    float s, c;

    Vec3 left, up;
    dir.OrthogonalBasis(left, up);
    left *= radius;
    up *= radius;

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = Math::TwoPi * i / numSteps;
                Math::SinCos16(a, s, c);
                *fvptr++ = origin + c * left + s * up;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        Vec3 lastPoint = origin + left;
        Vec3 point;

        for (int i = 1; i <= numSteps; i++) {
            float a = Math::TwoPi * i / numSteps;
            Math::SinCos16(a, s, c);
            point = origin + c * left + s * up;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }
    }
}

void RenderWorld::DebugHollowCircle(const Vec3 &origin, const Vec3 &dir, const float radius1, const float radius2, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    const int numSteps = 48;
    float s, c;

    Vec3 left[2], up[2];
    dir.OrthogonalBasis(left[0], up[0]);
    left[1] = left[0] * radius2;
    up[1] = up[0] * radius2;
    left[0] *= radius1;
    up[0] *= radius1;

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, (numSteps + 1) * 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            for (int i = 0; i <= numSteps; i++) {
                float a = Math::TwoPi * i / numSteps;
                Math::SinCos16(a, s, c);
                *fvptr++ = origin + c * left[0] + s * up[0];
                *fvptr++ = origin + c * left[1] + s * up[1];
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        for (int i = 0; i < 2; i++) {
            Vec3 lastPoint = origin + left[i];
            Vec3 point;

            for (int j = 1; j <= numSteps; j++) {
                float a = Math::TwoPi * j / numSteps;
                Math::SinCos16(a, s, c);
                point = origin + c * left[i] + s * up[i];
                DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
                lastPoint = point;
            }
        }
    }
}

void RenderWorld::DebugArc(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    float deltaAngle = angle2 - angle1;
    if (deltaAngle == 0.0f) {
        return;
    }

    int numSteps = Min((int)Math::Ceil((Math::Fabs(deltaAngle / 7.5f))), 96);
    float theta1 = DEG2RAD(angle1);
    float theta2 = DEG2RAD(angle2);
    float delta = theta2 - theta1;
    float s, c;

    Vec3 rx = radius * right;
    Vec3 ry = radius * up;

    if (drawSector && debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = theta1 + delta * i / numSteps;
                Math::SinCos16(a, s, c);
                *fvptr++ = origin + c * rx + s * ry;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        Math::SinCos(theta1, s, c);
        Vec3 lastPoint = origin + c * rx + s * ry;
        Vec3 point;

        if (drawSector) {
            DebugLine(origin, lastPoint, lineWidth, depthTest, lifeTime);
        }

        for (int i = 1; i <= numSteps; i++) {
            float a = theta1 + delta * i / numSteps;
            Math::SinCos16(a, s, c);
            point = origin + c * rx + s * ry;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }

        if (drawSector) {
            DebugLine(lastPoint, origin, lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugEllipse(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius1, const float radius2, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    const int numSteps = 64;
    float s, c;

    Vec3 rx = right * radius1;
    Vec3 ry = up * radius2;

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = Math::TwoPi * i / numSteps;
                Math::SinCos16(a, s, c);
                *fvptr++ = origin + c * rx + s * ry;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        Vec3 lastPoint = origin + rx;
        Vec3 point;

        for (int i = 1; i <= numSteps; i++) {
            float a = Math::TwoPi * i / numSteps;
            Math::SinCos16(a, s, c);
            point = origin + c * rx + s * ry;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }
    }
}

void RenderWorld::DebugHemisphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 raxis, p, lastp, last0;
    float s, c;
    int num = 360 / 15;

    Vec3 *lastArray = (Vec3 *)_alloca16(num * sizeof(Vec3));

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, (num + 1) * 2 * (num / 4), debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            lastArray[0] = origin + axis[2] * radius;
            for (int n = 1; n < num; n++) {
                lastArray[n] = lastArray[0];
            }

            for (int i = 15; i <= 90; i += 15) {
                Math::SinCos16(DEG2RAD(i), s, c);
                float cr = c * radius;
                float sr = s * radius;

                last0 = lastArray[0];

                int j = 15;
                for (int n = 0; j <= 360; j += 15, n++) {
                    Math::SinCos16(DEG2RAD(j), s, c);
                    raxis = axis[0] * c + axis[1] * s;
                    p = origin + cr * axis[2] + sr * raxis;

                    *fvptr++ = lastArray[n];
                    *fvptr++ = p;

                    lastArray[n] = p;
                }

                *fvptr++ = last0;
                *fvptr++ = lastArray[0];
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        lastArray[0] = origin + axis[2] * radius;
        for (int n = 1; n < num; n++) {
            lastArray[n] = lastArray[0];
        }

        for (int i = 15; i <= 90; i += 15) {
            Math::SinCos16(DEG2RAD(i), s, c);
            float cr = c * radius;
            float sr = s * radius;

            lastp = origin + cr * axis[2] + sr * axis[0];

            int j = 15;
            for (int n = 0; j <= 360; j += 15, n++) {
                Math::SinCos16(DEG2RAD(j), s, c);
                raxis = axis[0] * c + axis[1] * s;
                p = origin + cr * axis[2] + sr * raxis;

                DebugLine(lastp, p, lineWidth, depthTest, lifeTime);
                DebugLine(lastp, lastArray[n], lineWidth, depthTest, lifeTime);

                lastArray[n] = lastp;
                lastp = p;
            }
        }
    }
}

void RenderWorld::DebugHemisphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool depthTest, int lifeTime) {
    DebugCircle(origin, axis[2], radius, lineWidth, false, depthTest, lifeTime);
    DebugArc(origin, axis[0], axis[2], radius, 0, 180, false, lineWidth, false, depthTest, lifeTime);
    DebugArc(origin, axis[1], axis[2], radius, 0, 180, false, lineWidth, false, depthTest, lifeTime);
}

void RenderWorld::DebugSphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 raxis, p, lastp, last0;
    float s, c;
    int num = 360 / 15;

    Vec3 *lastArray = (Vec3 *)_alloca16(num * sizeof(Vec3));

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, (num + 1) * 2 * (num / 2), debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            lastArray[0] = origin + axis[2] * radius;
            for (int n = 1; n < num; n++) {
                lastArray[n] = lastArray[0];
            }

            for (int i = 15; i <= 180; i += 15) {
                Math::SinCos16(DEG2RAD(i), s, c);
                float cr = c * radius;
                float sr = s * radius;

                last0 = lastArray[0];

                int j = 15;
                for (int n = 0; j <= 360; j += 15, n++) {
                    Math::SinCos16(DEG2RAD(j), s, c);
                    raxis = axis[0] * c + axis[1] * s;
                    p = origin + cr * axis[2] + sr * raxis;

                    *fvptr++ = lastArray[n];
                    *fvptr++ = p;

                    lastArray[n] = p;
                }

                *fvptr++ = last0;
                *fvptr++ = lastArray[0];
            }
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        lastArray[0] = origin + axis[2] * radius;
        for (int n = 1; n < num; n++) {
            lastArray[n] = lastArray[0];
        }

        for (int i = 15; i <= 180; i += 15) {
            Math::SinCos16(DEG2RAD(i), s, c);
            float cr = c * radius;
            float sr = s * radius;

            lastp = origin + cr * axis[2] + sr * axis[0];

            int j = 15;
            for (int n = 0; j <= 360; j += 15, n++) {
                Math::SinCos16(DEG2RAD(j), s, c);
                raxis = axis[0] * c + axis[1] * s;
                p = origin + cr * axis[2] + sr * raxis;

                DebugLine(lastp, p, lineWidth, depthTest, lifeTime);
                DebugLine(lastp, lastArray[n], lineWidth, depthTest, lifeTime);

                lastArray[n] = lastp;
                lastp = p;
            }
        }
    }
}

void RenderWorld::DebugSphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool depthTest, int lifeTime) {
    DebugCircle(origin, axis[0], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(origin, axis[1], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(origin, axis[2], radius, lineWidth, false, depthTest, lifeTime);
}

void RenderWorld::DebugAABB(const AABB &aabb, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    if (aabb.IsCleared()) {
        return;
    }

    Vec3 v[8];
    aabb.ToPoints(v);

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, 14, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        for (int i = 0; i < 4; i++) {
            DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugOBB(const OBB &obb, float lineWidth, bool depthTest, bool twoSided, int lifeTime) {
    Vec3 v[8];

    obb.ToPoints(v);

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, 14, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        for (int i = 0; i < 4; i++) {
            DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugFrustum(const Frustum &frustum, const bool showFromOrigin, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 v[8];

    frustum.ToPoints(v);

    if (debugFillColor.a > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, 24, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor.a > 0) {
        if (frustum.GetNearDistance() > 0.0f ) {
            for (int i = 0; i < 4; i++) {
                DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            }

            if (showFromOrigin) {
                for (int i = 0; i < 4; i++ ) {
                    DebugLine(frustum.GetOrigin(), v[i], lineWidth, depthTest, lifeTime);
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

// radius1 is the radius at the apex
// radius2 is the radius at the bottom
void RenderWorld::DebugCone(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 apex = origin + axis[2] * height;
    Vec3 lastp2 = origin + radius2 * axis[0];
    Vec3 p1, p2, d;
    float s, c;

    Vec3 *fvptr = nullptr;

    if (radius1 == 0.0f) {
        if (debugFillColor.a > 0) {
            fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
            if (fvptr) {
                *fvptr++ = apex;

                for (int i = 0; i <= 360; i += 15) {
                    Math::SinCos16(DEG2RAD(i), s, c);
                    d = c * axis[0] + s * axis[1];
                    *fvptr++ = origin + d * radius2;
                }

                if (drawCap) {
                    fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = origin;

                    for (int i = 0; i <= 360; i += 15) {
                        Math::SinCos16(DEG2RAD(i), s, c);
                        d = c * axis[0] - s * axis[1];
                        *fvptr++ = origin + d * radius2;
                    }
                }
            }
        }

        if (debugLineColor.a > 0) {
            for (int i = 15; i <= 360; i += 15) {
                Math::SinCos16(DEG2RAD(i), s, c);
                d = c * axis[0] + s * axis[1];
                p2 = origin + d * radius2;

                DebugLine(lastp2, p2, lineWidth, depthTest, lifeTime);
                DebugLine(p2, apex, lineWidth, depthTest, lifeTime);

                lastp2 = p2;
            }
        }
    } else {
        Vec3 lastp1 = apex + radius1 * axis[0];

        if (debugFillColor.a > 0) {
            fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleStrip, (360 / 15) * 2 + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
            if (fvptr) {
                *fvptr++ = lastp1;
                *fvptr++ = lastp2;

                for (int i = 15; i <= 360; i += 15) {
                    Math::SinCos16(DEG2RAD(i), s, c);
                    d = c * axis[0] + s * axis[1];
                    *fvptr++ = apex + d * radius1;
                    *fvptr++ = origin + d * radius2;
                }

                if (drawCap) {
                    fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = apex;

                    for (int i = 0; i <= 360; i += 15) {
                        Math::SinCos16(DEG2RAD(i), s, c);
                        d = c * axis[0] - s * axis[1];
                        *fvptr++ = apex + d * radius1;
                    }

                    fvptr = RB_ReserveDebugPrimsVerts(RHI::Topology::TriangleFan, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = origin;

                    for (int i = 0; i <= 360; i += 15) {
                        Math::SinCos16(DEG2RAD(i), s, c);
                        d = c * axis[0] - s * axis[1];
                        *fvptr++ = origin + d * radius2;
                    }
                }
            }
        }

        if (lineWidth > 0 && debugLineColor.a > 0) {
            for (int i = 15; i <= 360; i += 15) {
                Math::SinCos16(DEG2RAD(i), s, c);
                d = c * axis[0] + s * axis[1];
                p1 = apex + d * radius1;
                p2 = origin + d * radius2;

                DebugLine(lastp1, p1, lineWidth, depthTest, lifeTime);
                DebugLine(lastp2, p2, lineWidth, depthTest, lifeTime);
                DebugLine(p1, p2, lineWidth, depthTest, lifeTime);

                lastp1 = p1;
                lastp2 = p2;
            }
        }
    }
}

void RenderWorld::DebugCone(const Vec3 &apex, const Vec3 &bottom, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    Mat3 axis;
    axis[2] = apex - bottom;
    float height = axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DebugCone(bottom, axis, height, radius1, radius2, drawCap, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugConeSimple(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 apex = origin + axis[2] * height;
    Vec3 vec[2][2];
    vec[0][0] = axis[0] * radius2;
    vec[0][1] = axis[1] * radius2;
    vec[1][0] = axis[0] * radius1;
    vec[1][1] = axis[1] * radius1;

    if (radius1 == 0.0f) {
        DebugCircle(apex, axis[2], radius1, lineWidth, twoSided, depthTest, lifeTime);
    }

    DebugCircle(origin, axis[2], radius2, lineWidth, twoSided, depthTest, lifeTime);

    DebugLine(origin + vec[0][0], apex + vec[1][0], lineWidth, depthTest, lifeTime);
    DebugLine(origin - vec[0][0], apex - vec[1][0], lineWidth, depthTest, lifeTime);
    DebugLine(origin + vec[0][1], apex + vec[1][1], lineWidth, depthTest, lifeTime);
    DebugLine(origin - vec[0][1], apex - vec[1][1], lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugCylinder(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    Vec3 origin = center - axis[2] * height * 0.5f;

    DebugCone(origin, axis, height, radius, radius, true, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCylinder(const Vec3 &top, const Vec3 &bottom, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    DebugCone(top, bottom, radius, radius, true, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCylinderSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 top = center + axis[2] * height * 0.5f;
    Vec3 bottom = center - axis[2] * height * 0.5f;

    DebugCircle(top, axis[2], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(bottom, -axis[2], radius, lineWidth, false, depthTest, lifeTime);

    Vec3 vec[2];
    vec[0] = axis[0] * radius;
    vec[1] = axis[1] * radius;

    DebugLine(bottom + vec[0], top + vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(bottom - vec[0], top - vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(bottom + vec[1], top + vec[1], lineWidth, depthTest, lifeTime);
    DebugLine(bottom - vec[1], top - vec[1], lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugCapsule(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 a = center + axis[2] * height * 0.5f;
    Vec3 b = center - axis[2] * height * 0.5f;

    DebugCone(a, axis, height, radius, radius, false, lineWidth, twoSided, depthTest, lifeTime);

    DebugHemisphere(a, axis, radius, lineWidth, twoSided, depthTest, lifeTime);

    Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DebugHemisphere(b, axis2, radius, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCapsule(const Vec3 &a, const Vec3 &b, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Mat3 axis;
    axis[2] = a - b;
    axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DebugCone(a, b, radius, radius, false, lineWidth, twoSided, depthTest, lifeTime);

    DebugHemisphere(a, axis, radius, lineWidth, twoSided, depthTest, lifeTime);

    axis[2] = -axis[2];
    axis[1] = -axis[1];
    DebugHemisphere(b, axis, radius, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCapsuleSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 a = center + axis[2] * height * 0.5f;
    Vec3 b = center - axis[2] * height * 0.5f;

    Vec3 vec[2];
    vec[0] = axis[0] * radius;
    vec[1] = axis[1] * radius;

    DebugLine(a + vec[0], b + vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(a - vec[0], b - vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(a + vec[1], b + vec[1], lineWidth, depthTest, lifeTime);
    DebugLine(a - vec[1], b - vec[1], lineWidth, depthTest, lifeTime);

    DebugHemisphereSimple(a, axis, radius, lineWidth, depthTest, lifeTime);

    Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DebugHemisphereSimple(b, axis2, radius, lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugArrow(const Vec3 &start, const Vec3 &end, float coneSize, float coneRadius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 dir = end - start;
    float length = dir.Normalize();
    coneSize = Min(coneSize, length);
    Vec3 coneBottom = end - dir * coneSize;

    Color4 _debugLineColor = debugLineColor;
    Color4 _debugFillColor = debugFillColor;

    SetDebugColor(Color4::zero, _debugFillColor);
    DebugCone(end, coneBottom, 0, coneRadius, false, 0, true, depthTest, lifeTime);

    SetDebugColor(Color4::zero, Color4(0, 0, 0, 0.75f));
    DebugCircle(coneBottom - dir * 0.0001f, -dir, coneRadius, 0, false, depthTest, lifeTime);

    if (lineWidth > 0 && _debugLineColor[3] > 0) {
        SetDebugColor(_debugLineColor, Color4::zero);
        DebugLine(start, coneBottom, lineWidth, depthTest, lifeTime);
    }

    SetDebugColor(_debugLineColor, _debugFillColor);
}

void RenderWorld::ClearDebugText(int time) {
    RB_ClearDebugText(time);
}

void RenderWorld::DebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth, const int align, bool depthTest, int lifeTime) {
    RB_AddDebugText(text, origin, viewAxis, scale, lineWidth, debugLineColor, align, lifeTime, depthTest);
}

void RenderWorld::DebugJoints(const RenderObject *renderObject, bool showJointsNames, const Mat3 &viewAxis) {
    const Joint *joint = renderObject->state.mesh->GetJoints();
    const Mat3x4 *jointMat = renderObject->state.joints;

    for (int i = 0; i < renderObject->state.numJoints; i++, joint++, jointMat++) {
        Vec3 pos = renderObject->state.worldMatrix.TransformPos(jointMat->ToTranslationVec3());
        Mat3 mat = jointMat->ToMat3();

        if (joint->parent) {
            int parentIndex = (int)(joint->parent - renderObject->state.mesh->GetJoints());
            SetDebugColor(Color4::white, Color4::zero);
            DebugLine(pos, renderObject->state.worldMatrix.TransformPos(renderObject->state.joints[parentIndex].ToTranslationVec3()), 1);
        }

        SetDebugColor(Color4::red, Color4::zero);
        DebugLine(pos, pos + renderObject->state.worldMatrix.ToMat3() * mat[0] * MeterToUnit(0.04f), 1);
        SetDebugColor(Color4::green, Color4::zero);
        DebugLine(pos, pos + renderObject->state.worldMatrix.ToMat3() * mat[1] * MeterToUnit(0.04f), 1);
        SetDebugColor(Color4::blue, Color4::zero);
        DebugLine(pos, pos + renderObject->state.worldMatrix.ToMat3() * mat[2] * MeterToUnit(0.04f), 1);

        if (showJointsNames) {
            SetDebugColor(Color4::white, Color4::zero);
            DebugText(joint->name.c_str(), pos, viewAxis, MeterToUnit(0.04f), 1, 0);
        }
    }
}

BE_NAMESPACE_END
