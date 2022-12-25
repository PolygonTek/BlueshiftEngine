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
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

/*
  bit 0 = min x
  bit 1 = max x
  bit 2 = min y
  bit 3 = max y
  bit 4 = min z
  bit 5 = max z
*/
static int boxVertPlanes[8] = {
    ((1<<0) | (1<<2) | (1<<4)),
    ((1<<1) | (1<<2) | (1<<4)),
    ((1<<1) | (1<<3) | (1<<4)),
    ((1<<0) | (1<<3) | (1<<4)),
    ((1<<0) | (1<<2) | (1<<5)),
    ((1<<1) | (1<<2) | (1<<5)),
    ((1<<1) | (1<<3) | (1<<5)),
    ((1<<0) | (1<<3) | (1<<5)),
};

static void BoxToPoints(const Vec3 &center, const Vec3 &extents, const Mat3 &axis, Vec3 points[8]) {
    Mat3 ax;
    ax[0] = extents[0] * axis[0];
    ax[1] = extents[1] * axis[1];
    ax[2] = extents[2] * axis[2];

    Vec3 temp[4];
    temp[0] = center - ax[0];
    temp[1] = center + ax[0];
    temp[2] = ax[1] - ax[2];
    temp[3] = ax[1] + ax[2];

    points[0] = temp[0] - temp[3];
    points[1] = temp[1] - temp[3];
    points[2] = temp[1] + temp[2];
    points[3] = temp[0] + temp[2];
    points[4] = temp[0] - temp[2];
    points[5] = temp[1] - temp[2];
    points[6] = temp[1] + temp[3];
    points[7] = temp[0] + temp[3];
}

float Frustum::PlaneDistance(const Plane &plane) const {
    float min, max;

    ProjectOnAxis(plane.normal, min, max);
    if (min + plane[3] > 0.0f) {
        return min + plane[3];
    }

    if (max + plane[3] < 0.0f) {
        return max + plane[3];
    }

    return 0.0f;
}

int Frustum::PlaneSide(const Plane &plane, const float epsilon) const {
    float min, max;

    ProjectOnAxis(plane.normal, min, max);
    if (min + plane[3] > epsilon) {
        return Plane::Side::Front;
    }

    if (max + plane[3] < epsilon) {
        return Plane::Side::Back;
    }

    return Plane::Side::Cross;
}

bool Frustum::CullPoint(const Vec3 &point) const {
    // transform point to frustum space
    Vec3 p = axis.TransposedMulVec(point - origin);
    // test whether or not the point is within the frustum
    if (p.x < dNear || p.x > dFar) {
        return true;
    }

    float scale = p.x * invFar;
    if (Math::Fabs(p.y) > dLeft * scale) {
        return true;
    }

    if (Math::Fabs(p.z) > dUp * scale) {
        return true;
    }

    return false;
}

// Tests if any of the planes of the frustum can be used as a separating plane.
//
//   3 muls best case
//  25 muls worst case
bool Frustum::CullLocalOBB(const Vec3 &localOrigin, const Vec3 &extents, const Mat3 &localAxis) const {
    // near plane
    float d1 = dNear - localOrigin.x;
    float d2 = Math::Fabs(extents[0] * localAxis[0][0]) + 
               Math::Fabs(extents[1] * localAxis[1][0]) + 
               Math::Fabs(extents[2] * localAxis[2][0]);
    if (d1 - d2 > 0.0f) {
        return true;
    }

    // far plane
    d1 = localOrigin.x - dFar;
    if (d1 - d2 > 0.0f) {
        return true;
    }

    Vec3 testOrigin = localOrigin;
    Mat3 testAxis = localAxis;

    if (testOrigin.y < 0.0f) {
        testOrigin.y = -testOrigin.y;
        testAxis[0][1] = -testAxis[0][1];
        testAxis[1][1] = -testAxis[1][1];
        testAxis[2][1] = -testAxis[2][1];
    }

    // test left/right planes
    d1 = dFar * testOrigin.y - dLeft * testOrigin.x;
    d2 = Math::Fabs(extents[0] * (dFar * testAxis[0][1] - dLeft * testAxis[0][0])) + 
         Math::Fabs(extents[1] * (dFar * testAxis[1][1] - dLeft * testAxis[1][0])) + 
         Math::Fabs(extents[2] * (dFar * testAxis[2][1] - dLeft * testAxis[2][0]));
    if (d1 - d2 > 0.0f) {
        return true;
    }

    if (testOrigin.z < 0.0f) {
        testOrigin.z = -testOrigin.z;
        testAxis[0][2] = -testAxis[0][2];
        testAxis[1][2] = -testAxis[1][2];
        testAxis[2][2] = -testAxis[2][2];
    }

    // test up/down planes
    d1 = dFar * testOrigin.z - dUp * testOrigin.x;
    d2 = Math::Fabs(extents[0] * (dFar * testAxis[0][2] - dUp * testAxis[0][0])) + 
         Math::Fabs(extents[1] * (dFar * testAxis[1][2] - dUp * testAxis[1][0])) + 
         Math::Fabs(extents[2] * (dFar * testAxis[2][2] - dUp * testAxis[2][0]));
    if (d1 - d2 > 0.0f) {
        return true;
    }

    return false;
}

// Tests if any of the planes of the frustum can be used as a separating plane.
//
//  24 muls best case
//  37 muls worst case
bool Frustum::CullAABB(const AABB &bounds) const {
    Vec3 center = (bounds[0] + bounds[1]) * 0.5f;
    Vec3 extents = bounds[1] - center;

    // transform the bounds into the space of this frustum
    Vec3 localOrigin = axis.TransposedMulVec(center - origin);
    Mat3 localAxis = axis.Transpose();

    return CullLocalOBB(localOrigin, extents, localAxis);
}

// Tests if any of the planes of the frustum can be used as a separating plane.
//
//  39 muls best case
//  61 muls worst case
bool Frustum::CullOBB(const OBB &box) const {
    // transform the box into the space of this frustum
    Vec3 localOrigin = axis.TransposedMulVec(box.Center() - origin);
    Mat3 localAxis = axis.TransposedMul(box.Axis());

    return CullLocalOBB(localOrigin, box.Extents(), localAxis);
}

// Tests if any of the planes of the frustum can be used as a separating plane.
//
//   9 muls best case
//  21 muls worst case
bool Frustum::CullSphere(const Sphere &sphere) const {
    Vec3 center = axis.TransposedMulVec(sphere.Center() - origin);
    float r = sphere.Radius();

    // test near plane
    if (dNear - center.x > r) {
        return true;
    }

    // test far plane
    if (center.x - dFar > r) {
        return true;
    }

    float rs = r * r;
    float sFar = dFar * dFar;

    // test left/right planes
    float d = dFar * Math::Fabs(center.y) - dLeft * center.x;
    if (d > 0) {
        if (d * d > rs * (sFar + dLeft * dLeft)) {
            return true;
        }
    }

    // test up/down planes
    d = dFar * Math::Fabs(center.z) - dUp * center.x;
    if (d > 0) {
        if ((d * d) > rs * (sFar + dUp * dUp)) {
            return true;
        }
    }

    return false;
}

// Tests if any of the planes of this frustum can be used as a separating plane.
//
//   0 muls best case
//  30 muls worst case
bool Frustum::CullLocalFrustum(const Frustum &localFrustum, const Vec3 indexPoints[8], const Vec3 cornerVecs[4]) const {
    int index;
    float dx, dy, dz, leftScale, upScale;

    // test near plane
    dy = -localFrustum.axis[1].x;
    dz = -localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].x < dNear) {
        return true;
    }

    // test far plane
    dy = localFrustum.axis[1].x;
    dz = localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].x > dFar) {
        return true;
    }

    leftScale = dLeft * invFar;

    // test left plane
    dy = dFar * localFrustum.axis[1].y - dLeft * localFrustum.axis[1].x;
    dz = dFar * localFrustum.axis[2].y - dLeft * localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = dFar * cornerVecs[index].y - dLeft * cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].y > indexPoints[index].x * leftScale) {
        return true;
    }

    // test right plane
    dy = -dFar * localFrustum.axis[1].y - dLeft * localFrustum.axis[1].x;
    dz = -dFar * localFrustum.axis[2].y - dLeft * localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -dFar * cornerVecs[index].y - dLeft * cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].y < -indexPoints[index].x * leftScale) {
        return true;
    }

    upScale = dUp * invFar;

    // test up plane
    dy = dFar * localFrustum.axis[1].z - dUp * localFrustum.axis[1].x;
    dz = dFar * localFrustum.axis[2].z - dUp * localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = dFar * cornerVecs[index].z - dUp * cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].z > indexPoints[index].x * upScale) {
        return true;
    }
    
    // test down plane
    dy = -dFar * localFrustum.axis[1].z - dUp * localFrustum.axis[1].x;
    dz = -dFar * localFrustum.axis[2].z - dUp * localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -dFar * cornerVecs[index].z - dUp * cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].z < -indexPoints[index].x * upScale) {
        return true;
    }

    return false;
}

// Tests if any of the planes of this frustum can be used as a separating plane.
//
//  58 muls best case
//  88 muls worst case
bool Frustum::CullFrustum(const Frustum &frustum) const {
    Frustum localFrustum;
    Vec3 indexPoints[8], cornerVecs[4];

    // transform the given frustum into the space of this frustum
    localFrustum = frustum;
    localFrustum.origin = axis.TransposedMulVec(frustum.origin - origin);
    localFrustum.axis = axis.TransposedMul(frustum.axis);

    localFrustum.ToIndexPointsAndCornerVecs(indexPoints, cornerVecs);

    return CullLocalFrustum(localFrustum, indexPoints, cornerVecs);
}

bool Frustum::CullLocalWinding(const Vec3 *points, const int numPoints, int *pointCull) const {
    float leftScale = dLeft * invFar;
    float upScale = dUp * invFar;

    int culled = -1;
    for (int i = 0; i < numPoints; i++) {
        const Vec3 &p = points[i];
        int pCull = 0;
        if (p.x < dNear) {
            pCull = 1;
        } else if (p.x > dFar) {
            pCull = 2;
        }

        if (Math::Fabs(p.y) > p.x * leftScale) {
            pCull |= 4 << IEEE_FLT_SIGNBITSET(p.y);
        }
        if (Math::Fabs(p.z) > p.x * upScale) {
            pCull |= 16 << IEEE_FLT_SIGNBITSET(p.z);
        }
        
        culled &= pCull;
        pointCull[i] = pCull;
    }

    return (culled != 0);
}

// Tests if any of the bounding box planes can be used as a separating plane.
bool Frustum::BoundsCullLocalFrustum(const AABB &bounds, const Frustum &localFrustum, const Vec3 indexPoints[8], const Vec3 cornerVecs[4]) const {
    int index;
    float dx, dy, dz;

    dy = -localFrustum.axis[1].x;
    dz = -localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].x < bounds[0].x) {
        return true;
    }

    dy = localFrustum.axis[1].x;
    dz = localFrustum.axis[2].x;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = cornerVecs[index].x;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].x > bounds[1].x) {
        return true;
    }

    dy = -localFrustum.axis[1].y;
    dz = -localFrustum.axis[2].y;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -cornerVecs[index].y;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].y < bounds[0].y) {
        return true;
    }

    dy = localFrustum.axis[1].y;
    dz = localFrustum.axis[2].y;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = cornerVecs[index].y;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].y > bounds[1].y) {
        return true;
    }

    dy = -localFrustum.axis[1].z;
    dz = -localFrustum.axis[2].z;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = -cornerVecs[index].z;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].z < bounds[0].z) {
        return true;
    }

    dy = localFrustum.axis[1].z;
    dz = localFrustum.axis[2].z;
    index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    dx = cornerVecs[index].z;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);

    if (indexPoints[index].z > bounds[1].z) {
        return true;
    }

    return false;
}

//  7 divs
//  30 muls
bool Frustum::LocalLineIntersection(const Vec3 &start, const Vec3 &end) const {
    float d1, d2, f, x;
    int startInside = 1;
    float leftScale = dLeft * invFar;
    float upScale = dUp * invFar;
    Vec3 dir = end - start;

    // test near plane
    if (dNear > 0.0f) {
        d1 = dNear - start.x;
        startInside &= IEEE_FLT_SIGNBITSET(d1);
        if (IEEE_FLT_NOTZERO(d1)) {
            d2 = dNear - end.x;
            if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
                f = d1 / (d1 - d2);
                if (Math::Fabs(start.y + f * dir.y) <= dNear * leftScale) {
                    if (Math::Fabs(start.z + f * dir.z) <= dNear * upScale) {
                        return true;
                    }
                }
            }
        }
    }

    // test far plane
    d1 = start.x - dFar;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    if (IEEE_FLT_NOTZERO(d1)) {
        d2 = end.x - dFar;
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            if (Math::Fabs(start.y + f * dir.y) <= dFar * leftScale) {
                if (Math::Fabs(start.z + f * dir.z) <= dFar * upScale) {
                    return true;
                }
            }
        }
    }

    float fstart = dFar * start.y;
    float fend = dFar * end.y;
    float lstart = dLeft * start.x;
    float lend = dLeft * end.x;

    // test left plane
    d1 = fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    if (IEEE_FLT_NOTZERO(d1)) {
        d2 = fend - lend;
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = start.x + f * dir.x;
            if (x >= dNear && x <= dFar) {
                if (Math::Fabs(start.z + f * dir.z) <= x * upScale) {
                    return true;
                }
            }
        }
    }

    // test right plane
    d1 = -fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    if (IEEE_FLT_NOTZERO(d1)) {
        d2 = -fend - lend;
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = start.x + f * dir.x;
            if (x >= dNear && x <= dFar) {
                if (Math::Fabs(start.z + f * dir.z) <= x * upScale) {
                    return true;
                }
            }
        }
    }

    fstart = dFar * start.z;
    fend = dFar * end.z;
    lstart = dUp * start.x;
    lend = dUp * end.x;

    // test up plane
    d1 = fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    if (IEEE_FLT_NOTZERO(d1)) {
        d2 = fend - lend;
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = start.x + f * dir.x;
            if (x >= dNear && x <= dFar) {
                if (Math::Fabs(start.y + f * dir.y) <= x * leftScale) {
                    return true;
                }
            }
        }
    }

    // test down plane
    d1 = -fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    if (IEEE_FLT_NOTZERO(d1)) {
        d2 = -fend - lend;
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = start.x + f * dir.x;
            if (x >= dNear && x <= dFar) {
                if (Math::Fabs(start.y + f * dir.y) <= x * leftScale) {
                    return true;
                }
            }
        }
    }

    return (startInside != 0);
}

// Returns true if the ray starts inside the frustum.
// If there was an intersection scale1 <= scale2
bool Frustum::LocalRayIntersection(const Vec3 &start, const Vec3 &dir, float &scale1, float &scale2) const {
    float d1, d2, f, x;
    int startInside = 1;
    float leftScale = dLeft * invFar;
    float upScale = dUp * invFar;
    Vec3 end = start + dir;

    scale1 = Math::Infinity;
    scale2 = -Math::Infinity;

    // test near plane
    if (dNear > 0.0f) {
        d1 = dNear - start.x;
        startInside &= IEEE_FLT_SIGNBITSET(d1);
        d2 = dNear - end.x;
        if (d1 != d2) {
            f = d1 / (d1 - d2);
            if (Math::Fabs(start.y + f * dir.y) <= dNear * leftScale) {
                if (Math::Fabs(start.z + f * dir.z) <= dNear * upScale) {
                    if (f < scale1) scale1 = f;
                    if (f > scale2) scale2 = f;
                }
            }
        }
    }

    // test far plane
    d1 = start.x - dFar;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    d2 = end.x - dFar;
    if (d1 != d2) {
        f = d1 / (d1 - d2);
        if (Math::Fabs(start.y + f * dir.y) <= dFar * leftScale) {
            if (Math::Fabs(start.z + f * dir.z) <= dFar * upScale) {
                if (f < scale1) scale1 = f;
                if (f > scale2) scale2 = f;
            }
        }
    }

    float fstart = dFar * start.y;
    float fend = dFar * end.y;
    float lstart = dLeft * start.x;
    float lend = dLeft * end.x;

    // test left plane
    d1 = fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    d2 = fend - lend;
    if (d1 != d2) {
        f = d1 / (d1 - d2);
        x = start.x + f * dir.x;
        if (x >= dNear && x <= dFar) {
            if (Math::Fabs(start.z + f * dir.z) <= x * upScale) {
                if (f < scale1) scale1 = f;
                if (f > scale2) scale2 = f;
            }
        }
    }

    // test right plane
    d1 = -fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    d2 = -fend - lend;
    if (d1 != d2) {
        f = d1 / (d1 - d2);
        x = start.x + f * dir.x;
        if (x >= dNear && x <= dFar) {
            if (Math::Fabs(start.z + f * dir.z) <= x * upScale) {
                if (f < scale1) scale1 = f;
                if (f > scale2) scale2 = f;
            }
        }
    }

    fstart = dFar * start.z;
    fend = dFar * end.z;
    lstart = dUp * start.x;
    lend = dUp * end.x;

    // test up plane
    d1 = fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    d2 = fend - lend;
    if (d1 != d2) {
        f = d1 / (d1 - d2);
        x = start.x + f * dir.x;
        if (x >= dNear && x <= dFar) {
            if (Math::Fabs(start.y + f * dir.y) <= x * leftScale) {
                if (f < scale1) scale1 = f;
                if (f > scale2) scale2 = f;
            }
        }
    }

    // test down plane
    d1 = -fstart - lstart;
    startInside &= IEEE_FLT_SIGNBITSET(d1);
    d2 = -fend - lend;
    if (d1 != d2) {
        f = d1 / (d1 - d2);
        x = start.x + f * dir.x;
        if (x >= dNear && x <= dFar) {
            if (Math::Fabs(start.y + f * dir.y) <= x * leftScale) {
                if (f < scale1) scale1 = f;
                if (f > scale2) scale2 = f;
            }
        }
    }

    return (startInside != 0);
}

bool Frustum::IsContainPoint(const Vec3 &point) const {
    return !CullPoint(point);
}

bool Frustum::LocalFrustumIntersectsFrustum(const Vec3 points[8], const bool testFirstSide) const {
    int i;

    // test if any edges of the other frustum intersect this frustum
    for (i = 0; i < 4; i++) {
        if (LocalLineIntersection(points[i], points[4+i])) {
            return true;
        }
    }

    if (testFirstSide) {
        for (i = 0; i < 4; i++) {
            if (LocalLineIntersection(points[i], points[(i+1)&3])) {
                return true;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        if (LocalLineIntersection(points[4+i], points[4+((i+1)&3)])) {
            return true;
        }
    }

    return false;
}

bool Frustum::LocalFrustumIntersectsBounds(const Vec3 points[8], const AABB &bounds) const {
    int i;

    // test if any edges of the other frustum intersect this frustum
    for (i = 0; i < 4; i++) {
        if (bounds.IsIntersectLine(points[i], points[4+i])) {
            return true;
        }
    }

    if (dNear > 0.0f) {
        for (i = 0; i < 4; i++) {
            if (bounds.IsIntersectLine(points[i], points[(i+1)&3])) {
                return true;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        if (bounds.IsIntersectLine(points[4+i], points[4+((i+1)&3)])) {
            return true;
        }
    }

    return false;
}

bool Frustum::IsIntersectAABB(const AABB &bounds) const {
    Vec3 localOrigin, center, extents;
    Mat3 localAxis;

    center = (bounds[0] + bounds[1]) * 0.5f;
    extents = bounds[1] - center;

    localOrigin = axis.TransposedMulVec(center - origin);
    localAxis = axis.Transpose();

    if (CullLocalOBB(localOrigin, extents, localAxis)) {
        return false;
    }

    Vec3 indexPoints[8], cornerVecs[4];

    ToIndexPointsAndCornerVecs(indexPoints, cornerVecs);

    if (BoundsCullLocalFrustum(bounds, *this, indexPoints, cornerVecs)) {
        return false;
    }

    Swap(indexPoints[2], indexPoints[3]);
    Swap(indexPoints[6], indexPoints[7]);

    if (LocalFrustumIntersectsBounds(indexPoints, bounds)) {
        return true;
    }

    BoxToPoints(localOrigin, extents, localAxis, indexPoints);

    if (LocalFrustumIntersectsFrustum(indexPoints, true)) {
        return true;
    }

    return false;
}

bool Frustum::IsIntersectOBB(const OBB &box) const {
    Vec3 localOrigin;
    Mat3 localAxis;

    localOrigin = axis.TransposedMulVec(box.Center() - origin);
    localAxis = axis.TransposedMul(box.Axis());

    if (CullLocalOBB(localOrigin, box.Extents(), localAxis)) {
        return false;
    }

    Vec3 indexPoints[8], cornerVecs[4];
    Frustum localFrustum;

    localFrustum = *this;
    localFrustum.origin = box.Axis().TransposedMulVec(origin - box.Center());
    localFrustum.axis = box.Axis().TransposedMul(axis);
    localFrustum.ToIndexPointsAndCornerVecs(indexPoints, cornerVecs);

    if (BoundsCullLocalFrustum(AABB(-box.Extents(), box.Extents()), localFrustum, indexPoints, cornerVecs)) {
        return false;
    }

    Swap(indexPoints[2], indexPoints[3]);
    Swap(indexPoints[6], indexPoints[7]);

    if (LocalFrustumIntersectsBounds(indexPoints, AABB(-box.Extents(), box.Extents()))) {
        return true;
    }

    BoxToPoints(localOrigin, box.Extents(), localAxis, indexPoints);

    if (LocalFrustumIntersectsFrustum(indexPoints, true)) {
        return true;
    }

    return false;
}

// FIXME: test this
#define VORONOI_INDEX(x, y, z)	(x + y * 3 + z * 9)

bool Frustum::IsIntersectSphere(const Sphere &sphere) const {
    int index, x, y, z;
    float scale, r, d;
    Vec3 p, points[8];

    if (CullSphere(sphere)) {
        return false;
    }

    x = y = z = 0;
    Vec3 dir = Vec3::FromScalar(0);

    p = axis.TransposedMulVec(sphere.Center() - origin);

    if (p.x <= dNear) {
        scale = dNear * invFar;
        dir.y = Math::Fabs(p.y) - dLeft * scale;
        dir.z = Math::Fabs(p.z) - dUp * scale;
    } else if (p.x >= dFar) {
        dir.y = Math::Fabs(p.y) - dLeft;
        dir.z = Math::Fabs(p.z) - dUp;
    } else {
        scale = p.x * invFar;
        dir.y = Math::Fabs(p.y) - dLeft * scale;
        dir.z = Math::Fabs(p.z) - dUp * scale;
    }

    if (dir.y > 0.0f) {
        y = (1 + IEEE_FLT_SIGNBITNOTSET(p.y));
    }

    if (dir.z > 0.0f) {
        z = (1 + IEEE_FLT_SIGNBITNOTSET(p.z));
    }

    if (p.x < dNear) {
        scale = dLeft * dNear * invFar;
        if (p.x < dNear + (scale - p.y) * scale * invFar) {
            scale = dUp * dNear * invFar;
            if (p.x < dNear + (scale - p.z) * scale * invFar) {
                x = 1;
            }
        }
    } else {
        if (p.x > dFar) {
            x = 2;
        } else if (p.x > dFar + (dLeft - p.y) * dLeft * invFar) {
            x = 2;
        } else if (p.x > dFar + (dUp - p.z) * dUp * invFar) {
            x = 2;
        }
    }

    r = sphere.Radius();
    index = VORONOI_INDEX(x, y, z);
    switch (index) {
    case VORONOI_INDEX(0, 0, 0): return true;
    case VORONOI_INDEX(1, 0, 0): return (dNear - p.x < r);
    case VORONOI_INDEX(2, 0, 0): return (p.x - dFar < r);
    case VORONOI_INDEX(0, 1, 0): d = dFar * p.y - dLeft * p.x; return (d * d < r * r * (dFar * dFar + dLeft * dLeft));
    case VORONOI_INDEX(0, 2, 0): d = -dFar * p.z - dLeft * p.x; return (d * d < r * r * (dFar * dFar + dLeft * dLeft));
    case VORONOI_INDEX(0, 0, 1): d = dFar * p.z - dUp * p.x; return (d * d < r * r * (dFar * dFar + dUp * dUp));
    case VORONOI_INDEX(0, 0, 2): d = -dFar * p.z - dUp * p.x; return (d * d < r * r * (dFar * dFar + dUp * dUp));
    default:
        ToIndexPoints(points);
        switch(index) {
        case VORONOI_INDEX(1, 1, 1): return sphere.IsContainPoint(points[0]);
        case VORONOI_INDEX(2, 1, 1): return sphere.IsContainPoint(points[4]);
        case VORONOI_INDEX(1, 2, 1): return sphere.IsContainPoint(points[1]);
        case VORONOI_INDEX(2, 2, 1): return sphere.IsContainPoint(points[5]);
        case VORONOI_INDEX(1, 1, 2): return sphere.IsContainPoint(points[2]);
        case VORONOI_INDEX(2, 1, 2): return sphere.IsContainPoint(points[6]);
        case VORONOI_INDEX(1, 2, 2): return sphere.IsContainPoint(points[3]);
        case VORONOI_INDEX(2, 2, 2): return sphere.IsContainPoint(points[7]);
        case VORONOI_INDEX(1, 1, 0): return sphere.IsIntersectLine(points[0], points[2]);
        case VORONOI_INDEX(2, 1, 0): return sphere.IsIntersectLine(points[4], points[6]);
        case VORONOI_INDEX(1, 2, 0): return sphere.IsIntersectLine(points[1], points[3]);
        case VORONOI_INDEX(2, 2, 0): return sphere.IsIntersectLine(points[5], points[7]);
        case VORONOI_INDEX(1, 0, 1): return sphere.IsIntersectLine(points[0], points[1]);
        case VORONOI_INDEX(2, 0, 1): return sphere.IsIntersectLine(points[4], points[5]);
        case VORONOI_INDEX(0, 1, 1): return sphere.IsIntersectLine(points[0], points[4]);
        case VORONOI_INDEX(0, 2, 1): return sphere.IsIntersectLine(points[1], points[5]);
        case VORONOI_INDEX(1, 0, 2): return sphere.IsIntersectLine(points[2], points[3]);
        case VORONOI_INDEX(2, 0, 2): return sphere.IsIntersectLine(points[6], points[7]);
        case VORONOI_INDEX(0, 1, 2): return sphere.IsIntersectLine(points[2], points[6]);
        case VORONOI_INDEX(0, 2, 2): return sphere.IsIntersectLine(points[3], points[7]);
        }
        break;
    }
    return false;
}

bool Frustum::IsIntersectFrustum(const Frustum &frustum) const {
    Vec3 indexPoints2[8], cornerVecs2[4];
    Frustum localFrustum2;

    localFrustum2 = frustum;
    localFrustum2.origin = axis.TransposedMulVec(frustum.origin - origin);
    localFrustum2.axis = axis.TransposedMul(frustum.axis);
    localFrustum2.ToIndexPointsAndCornerVecs(indexPoints2, cornerVecs2);

    if (CullLocalFrustum(localFrustum2, indexPoints2, cornerVecs2)) {
        return false;
    }

    Vec3 indexPoints1[8], cornerVecs1[4];
    Frustum localFrustum1;

    localFrustum1 = *this;
    localFrustum1.origin = frustum.axis.TransposedMulVec(origin - frustum.origin);
    localFrustum1.axis = frustum.axis.TransposedMul(axis);
    localFrustum1.ToIndexPointsAndCornerVecs(indexPoints1, cornerVecs1);

    if (frustum.CullLocalFrustum(localFrustum1, indexPoints1, cornerVecs1)) {
        return false;
    }
    
    Swap(indexPoints2[2], indexPoints2[3]);
    Swap(indexPoints2[6], indexPoints2[7]);

    if (LocalFrustumIntersectsFrustum(indexPoints2, (localFrustum2.dNear > 0.0f))) {
        return true;
    }
    
    Swap(indexPoints1[2], indexPoints1[3]);
    Swap(indexPoints1[6], indexPoints1[7]);

    if (frustum.LocalFrustumIntersectsFrustum(indexPoints1, (localFrustum1.dNear > 0.0f))) {
        return true;
    }

    return false;
}

// Returns true if the line intersects the box between the start and end point.
bool Frustum::IsIntersectLine(const Vec3 &start, const Vec3 &end) const {
    return LocalLineIntersection(axis.TransposedMulVec(start - origin), axis.TransposedMulVec(end - origin));
}

bool Frustum::IntersectRay(const Ray &ray, float *hitDistMin, float *hitDistMax) const {
    Vec3 localOrigin = axis.TransposedMulVec(ray.origin - origin);
    Vec3 localDir = axis.TransposedMulVec(ray.dir);
    float scale1, scale2;

    if (!LocalRayIntersection(localOrigin, localDir, scale1, scale2)) {
        if (scale1 > scale2) {
            return false;
        }
    }
    if (hitDistMin) {
        *hitDistMin = scale1;
    }
    if (hitDistMax) {
        *hitDistMax = scale2;
    }
    return true;
}

float Frustum::IntersectRay(const Ray &ray) const {
    float hitDistMin, hitDistMax;

    if (IntersectRay(ray, &hitDistMin, &hitDistMax)) {
        return hitDistMin;
    }
    return FLT_MAX;
}

// Creates a frustum which contains the projection of the AABB.
bool Frustum::FromProjection(const AABB &bounds, const Vec3 &projectionOrigin, const float dFar) {
    return FromProjection(OBB(bounds, Vec3::origin, Mat3::identity), projectionOrigin, dFar);
}

// Creates a frustum which contains the projection of the OBB.
bool Frustum::FromProjection(const OBB &box, const Vec3 &projectionOrigin, const float dFar) {
    int i, bestAxis;
    float value, bestValue;
    Vec3 dir;

    assert(dFar > 0.0f);

    this->dNear = this->dFar = this->invFar = 0.0f;

    dir = box.Center() - projectionOrigin;
    if (dir.Normalize() == 0.0f) {
        return false;
    }
    
    bestAxis = 0;
    bestValue = Math::Fabs(box.Axis()[0].Dot(dir));
    for (i = 1; i < 3; i++) {
        value = Math::Fabs(box.Axis()[i].Dot(dir));
        if (value * box.Extents()[bestAxis] * box.Extents()[bestAxis] < bestValue * box.Extents()[i] * box.Extents()[i]) {
            bestValue = value;
            bestAxis = i;
        }
    }

#if 1

    int j, minX, minY, maxY, minZ, maxZ;
    Vec3 points[8];

    minX = minY = maxY = minZ = maxZ = 0;

    for (j = 0; j < 2; j++) {
        axis[0] = dir;
        axis[1] = (box.Axis()[bestAxis] - (box.Axis()[bestAxis] * axis[0]) * axis[0]).Normalized();
        axis[2].SetFromCross(axis[0], axis[1]);

        BoxToPoints(axis.TransposedMulVec(box.Center() - projectionOrigin), box.Extents(), axis.TransposedMul(box.Axis()), points);

        if (points[0].x <= 1.0f) {
            return false;
        }

        minX = minY = maxY = minZ = maxZ = 0;
        for (i = 1; i < 8; i++) {
            if (points[i].x <= 1.0f) {
                return false;
            }

            if (points[i].x < points[minX].x) {
                minX = i;
            }

            if (points[minY].x * points[i].y < points[i].x * points[minY].y) {
                minY = i;
            } else if (points[maxY].x * points[i].y > points[i].x * points[maxY].y) {
                maxY = i;
            }
            
            if (points[minZ].x * points[i].z < points[i].x * points[minZ].z) {
                minZ = i;
            } else if (points[maxZ].x * points[i].z > points[i].x * points[maxZ].z) {
                maxZ = i;
            }
        }

        if (j == 0) {
            dir += Math::Tan16(0.5f * (Math::ATan16(points[minY].y, points[minY].x) + Math::ATan16(points[maxY].y, points[maxY].x))) * axis[1];
            dir += Math::Tan16(0.5f * (Math::ATan16(points[minZ].z, points[minZ].x) + Math::ATan16(points[maxZ].z, points[maxZ].x))) * axis[2];
            dir.Normalize();
        }
    }

    this->origin = projectionOrigin;
    this->dNear = points[minX].x;
    this->dFar = dFar;
    this->dLeft = Max(Math::Fabs(points[minY].y / points[minY].x), Math::Fabs(points[maxY].y / points[maxY].x)) * dFar;
    this->dUp = Max(Math::Fabs(points[minZ].z / points[minZ].x), Math::Fabs(points[maxZ].z / points[maxZ].x)) * dFar;
    this->invFar = 1.0f / dFar;

#elif 1

    int j;
    float f, x;
    AABB b;
    Vec3 points[8];

    for (j = 0; j < 2; j++) {
        axis[0] = dir;
        axis[1] = (box.Axis()[bestAxis] - (box.Axis()[bestAxis] * axis[0]) * axis[0]).Normalized();
        axis[2].Cross(axis[0], axis[1]);

        BoxToPoints(axis.TransposedMulVec(box.Center() - projectionOrigin), box.Extents(), axis.TransposedMul(box.Axis()), points);

        b.Clear();
        for (i = 0; i < 8; i++) {
            x = points[i].x;
            if (x <= 1.0f) {
                return false;
            }

            f = 1.0f / x;
            points[i].y *= f;
            points[i].z *= f;
            b.AddPoint(points[i]);
        }

        if (j == 0) {
            dir += Math::Tan16(0.5f * (Math::ATan16(b[1][1]) + Math::ATan16(b[0][1]))) * axis[1];
            dir += Math::Tan16(0.5f * (Math::ATan16(b[1][2]) + Math::ATan16(b[0][2]))) * axis[2];
            dir.Normalize();
        }
    }

    this->origin = projectionOrigin;
    this->dNear = b[0][0];
    this->dFar = dFar;
    this->dLeft = Max(Math::Fabs(b[0][1]), Math::Fabs(b[1][1])) * dFar;
    this->dUp = Max(Math::Fabs(b[0][2]), Math::Fabs(b[1][2])) * dFar;
    this->invFar = 1.0f / dFar;

#else

    float dist;
    Vec3 org;

    axis[0] = dir;
    axis[1] = (box.Axis()[bestAxis] - (box.Axis()[bestAxis] * axis[0]) * axis[0]).Normalized();
    axis[2].Cross(axis[0], axis[1]);

    for (i = 0; i < 3; i++) {
        dist[i] = Math::Fabs(box.Extents()[0] * (axis[i] * box.Axis()[0])) + 
                  Math::Fabs(box.Extents()[1] * (axis[i] * box.Axis()[1])) +
                  Math::Fabs(box.Extents()[2] * (axis[i] * box.Axis()[2]));
    }

    dist[0] = axis[0] * (box.Center() - projectionOrigin) - dist[0];
    if (dist[0] <= 1.0f) {
        return false;
    }

    float invDist = 1.0f / dist[0];

    this->origin = projectionOrigin;
    this->dNear = dist[0];
    this->dFar = dFar;
    this->dLeft = dist[1] * invDist * dFar;
    this->dUp = dist[2] * invDist * dFar;
    this->invFar = 1.0f / dFar;

#endif

    return true;
}

// Creates a frustum which contains the projection of the sphere.
bool Frustum::FromProjection(const Sphere &sphere, const Vec3 &projectionOrigin, const float dFar) {
    Vec3 dir;
    float d, r, s, x, y;

    assert(dFar > 0.0f);

    dir = sphere.Center() - projectionOrigin;
    d = dir.Normalize();
    r = sphere.Radius();

    if (d <= r + 1.0f) {
        this->dNear = this->dFar = this->invFar = 0.0f;
        return false;
    }

    origin = projectionOrigin;
    axis = dir.ToMat3();

    s = Math::Sqrt(d * d - r * r);
    x = r / d * s;
    y = Math::Sqrt(s * s - x * x);

    this->dNear = d - r;
    this->dFar = dFar;
    this->dLeft = x / y * dFar;
    this->dUp = dLeft;
    this->invFar = 1.0f / dFar;

    return true;
}

// Returns false if no part of the bounds extends beyond the near plane.
bool Frustum::ConstrainToAABB(const AABB &bounds) {
    float min, max, newdFar;

    bounds.ProjectOnAxis(axis[0], min, max);
    newdFar = max - axis[0].Dot(origin);
    if (newdFar <= dNear) {
        MoveFarDistance(dNear + 1.0f);
        return false;
    }

    if (newdFar < dFar) {
        MoveFarDistance(newdFar);
    }
    return true;
}

// Returns false if no part of the box extends beyond the near plane.
bool Frustum::ConstrainToOBB(const OBB &box) {
    float min, max, newdFar;

    box.ProjectOnAxis(axis[0], min, max);
    newdFar = max - axis[0].Dot(origin);
    if (newdFar <= dNear) {
        MoveFarDistance(dNear + 1.0f);
        return false;
    }

    if (newdFar < dFar) {
        MoveFarDistance(newdFar);
    }
    return true;
}

// Returns false if no part of the sphere extends beyond the near plane.
bool Frustum::ConstrainToSphere(const Sphere &sphere) {
    float min, max, newdFar;

    sphere.ProjectOnAxis(axis[0], min, max);
    newdFar = max - axis[0].Dot(origin);
    if (newdFar <= dNear) {
        MoveFarDistance(dNear + 1.0f);
        return false;
    }

    if (newdFar < dFar) {
        MoveFarDistance(newdFar);
    }
    return true;
}

// Returns false if no part of the frustum extends beyond the near plane.
bool Frustum::ConstrainToFrustum(const Frustum &frustum) {
    float min, max, newdFar;

    frustum.ProjectOnAxis(axis[0], min, max);
    newdFar = max - axis[0].Dot(origin);
    if (newdFar <= dNear) {
        MoveFarDistance(dNear + 1.0f);
        return false;
    }

    if (newdFar < dFar) {
        MoveFarDistance(newdFar);
    }
    return true;
}

OBB Frustum::ToOBB() const {
    float h = (dFar - dNear) * 0.5f;
    Vec3 extents(h, dLeft, dUp);
    OBB obb(origin + axis[0] * (dNear + h), extents, axis);
    return obb;
}

// http://www.gamedev.net/topic/604797-minimum-bounding-sphere-of-a-frustum/
Sphere Frustum::ToMinimumSphere() const {
    float u = dUp * invFar;
    float r = dLeft * invFar;
    float u2pr2 = u * u + r * r;
    float s = (dNear + dFar) * 0.5f * (1.0f + u2pr2);
    float radius;

    if (s >= dFar) {
        s = dFar;
        radius = dFar * Math::Sqrt(u2pr2);
    } else {
        float diff = 1.0f - s / dFar;
        radius = dFar * Math::Sqrt(diff * diff + u2pr2);
    }

    return Sphere(origin + s * axis[0], radius);
}

// planes point outwards
void Frustum::ToPlanes(Plane planes[6]) const {
    int i;
    Vec3 scaled[2];
    Vec3 points[4];

    planes[0].normal = -axis[0];
    planes[0].offset = dNear;
    planes[1].normal = axis[0];
    planes[1].offset = -dFar;

    scaled[0] = axis[1] * dLeft;
    scaled[1] = axis[2] * dUp;
    points[0] = scaled[0] + scaled[1];
    points[1] = -scaled[0] + scaled[1];
    points[2] = -scaled[0] - scaled[1];
    points[3] = scaled[0] - scaled[1];

    for (i = 0; i < 4; i++) {
        planes[i+2].normal = points[i].Cross(points[(i+1)&3] - points[i]);
        planes[i+2].Normalize();
        planes[i+2].FitThroughPoint(points[i]);
    }
}

void Frustum::ToPoints(Vec3 points[8]) const {
    Mat3 scaled;

    scaled[0] = origin + axis[0] * dNear;
    scaled[1] = axis[1] * (dLeft * dNear * invFar);
    scaled[2] = axis[2] * (dUp * dNear * invFar);

    points[0] = scaled[0] + scaled[1];
    points[1] = scaled[0] - scaled[1];
    points[2] = points[1] - scaled[2];
    points[3] = points[0] - scaled[2];
    points[0] += scaled[2];
    points[1] += scaled[2];

    scaled[0] = origin + axis[0] * dFar;
    scaled[1] = axis[1] * dLeft;
    scaled[2] = axis[2] * dUp;

    points[4] = scaled[0] + scaled[1];
    points[5] = scaled[0] - scaled[1];
    points[6] = points[5] - scaled[2];
    points[7] = points[4] - scaled[2];
    points[4] += scaled[2];
    points[5] += scaled[2];
}

void Frustum::ToClippedPoints(const float fractions[4], Vec3 points[8]) const {
    Mat3 scaled;

    scaled[0] = origin + axis[0] * dNear;
    scaled[1] = axis[1] * (dLeft * dNear * invFar);
    scaled[2] = axis[2] * (dUp * dNear * invFar);

    points[0] = scaled[0] + scaled[1];
    points[1] = scaled[0] - scaled[1];
    points[2] = points[1] - scaled[2];
    points[3] = points[0] - scaled[2];
    points[0] += scaled[2];
    points[1] += scaled[2];

    scaled[0] = axis[0] * dFar;
    scaled[1] = axis[1] * dLeft;
    scaled[2] = axis[2] * dUp;

    points[4] = scaled[0] + scaled[1];
    points[5] = scaled[0] - scaled[1];
    points[6] = points[5] - scaled[2];
    points[7] = points[4] - scaled[2];
    points[4] += scaled[2];
    points[5] += scaled[2];

    points[4] = origin + fractions[0] * points[4];
    points[5] = origin + fractions[1] * points[5];
    points[6] = origin + fractions[2] * points[6];
    points[7] = origin + fractions[3] * points[7];
}

void Frustum::ToIndexPoints(Vec3 indexPoints[8]) const {
    Mat3 scaled;

    scaled[0] = origin + axis[0] * dNear;
    scaled[1] = axis[1] * (dLeft * dNear * invFar);
    scaled[2] = axis[2] * (dUp * dNear * invFar);

    indexPoints[0] = scaled[0] - scaled[1];
    indexPoints[2] = scaled[0] + scaled[1];
    indexPoints[1] = indexPoints[0] + scaled[2];
    indexPoints[3] = indexPoints[2] + scaled[2];
    indexPoints[0] -= scaled[2];
    indexPoints[2] -= scaled[2];

    scaled[0] = origin + axis[0] * dFar;
    scaled[1] = axis[1] * dLeft;
    scaled[2] = axis[2] * dUp;

    indexPoints[4] = scaled[0] - scaled[1];
    indexPoints[6] = scaled[0] + scaled[1];
    indexPoints[5] = indexPoints[4] + scaled[2];
    indexPoints[7] = indexPoints[6] + scaled[2];
    indexPoints[4] -= scaled[2];
    indexPoints[6] -= scaled[2];
}

// 22 muls
void Frustum::ToIndexPointsAndCornerVecs(Vec3 indexPoints[8], Vec3 cornerVecs[4]) const {
    Mat3 scaled;

    scaled[0] = origin + axis[0] * dNear;
    scaled[1] = axis[1] * (dLeft * dNear * invFar);
    scaled[2] = axis[2] * (dUp * dNear * invFar);

    indexPoints[0] = scaled[0] - scaled[1];
    indexPoints[2] = scaled[0] + scaled[1];
    indexPoints[1] = indexPoints[0] + scaled[2];
    indexPoints[3] = indexPoints[2] + scaled[2];
    indexPoints[0] -= scaled[2];
    indexPoints[2] -= scaled[2];

    scaled[0] = axis[0] * dFar;
    scaled[1] = axis[1] * dLeft;
    scaled[2] = axis[2] * dUp;

    cornerVecs[0] = scaled[0] - scaled[1];
    cornerVecs[2] = scaled[0] + scaled[1];
    cornerVecs[1] = cornerVecs[0] + scaled[2];
    cornerVecs[3] = cornerVecs[2] + scaled[2];
    cornerVecs[0] -= scaled[2];
    cornerVecs[2] -= scaled[2];

    indexPoints[4] = cornerVecs[0] + origin;
    indexPoints[5] = cornerVecs[1] + origin;
    indexPoints[6] = cornerVecs[2] + origin;
    indexPoints[7] = cornerVecs[3] + origin;
}

// 18 muls
void Frustum::ProjectOnAxis(const Vec3 indexPoints[8], const Vec3 cornerVecs[4], const Vec3 &axis, float &min, float &max) const {
    float dy = axis.x * this->axis[1].x + axis.y * this->axis[1].y + axis.z * this->axis[1].z;
    float dz = axis.x * this->axis[2].x + axis.y * this->axis[2].y + axis.z * this->axis[2].z;
    int index = (IEEE_FLT_SIGNBITSET(dy) << 1) | IEEE_FLT_SIGNBITSET(dz);
    float dx = axis.x * cornerVecs[index].x + axis.y * cornerVecs[index].y + axis.z * cornerVecs[index].z;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);
    min = indexPoints[index].Dot(axis);
    index = ~index & 3;
    dx = -axis.x * cornerVecs[index].x - axis.y * cornerVecs[index].y - axis.z * cornerVecs[index].z;
    index |= (IEEE_FLT_SIGNBITSET(dx) << 2);
    max = indexPoints[index].Dot(axis);
}

// 40 muls
void Frustum::ProjectOnAxis(const Vec3 &axis, float &min, float &max) const {
    Vec3 indexPoints[8], cornerVecs[4];

    ToIndexPointsAndCornerVecs(indexPoints, cornerVecs);
    ProjectOnAxis(indexPoints, cornerVecs, axis, min, max);
}

// 76 muls
void Frustum::ProjectOnAxis(const Mat3 &axis, AABB &minmaxs) const {
    Vec3 indexPoints[8], cornerVecs[4];

    ToIndexPointsAndCornerVecs(indexPoints, cornerVecs);
    ProjectOnAxis(indexPoints, cornerVecs, axis[0], minmaxs[0][0], minmaxs[1][0]);
    ProjectOnAxis(indexPoints, cornerVecs, axis[1], minmaxs[0][1], minmaxs[1][1]);
    ProjectOnAxis(indexPoints, cornerVecs, axis[2], minmaxs[0][2], minmaxs[1][2]);
}

void Frustum::AddLocalLineToProjectionBoundsSetCull(const Vec3 &start, const Vec3 &end, int &startCull, int &endCull, AABB &bounds) const {
    Vec3 dir, p;
    float d1, d2, fstart, fend, lstart, lend, f;
    float leftScale, upScale;
    int cull1, cull2;

    leftScale = dLeft * invFar;
    upScale = dUp * invFar;
    dir = end - start;

    fstart = dFar * start.y;
    fend = dFar * end.y;
    lstart = dLeft * start.x;
    lend = dLeft * end.x;

    // test left plane
    d1 = -fstart + lstart;
    d2 = -fend + lend;
    cull1 = IEEE_FLT_SIGNBITSET(d1);
    cull2 = IEEE_FLT_SIGNBITSET(d2);
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (p.x > 0.0f) {
                p.z = start.z + f * dir.z;
                if (Math::Fabs(p.z) <= p.x * upScale) {
                    p.y = 1.0f;
                    p.z = p.z * dFar / (p.x * dUp);
                    bounds.AddPoint(p);
                }
            }
        }
    }

    // test right plane
    d1 = fstart + lstart;
    d2 = fend + lend;
    cull1 |= IEEE_FLT_SIGNBITSET(d1) << 1;
    cull2 |= IEEE_FLT_SIGNBITSET(d2) << 1;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (p.x > 0.0f) {
                p.z = start.z + f * dir.z;
                if (Math::Fabs(p.z) <= p.x * upScale) {
                    p.y = -1.0f;
                    p.z = p.z * dFar / (p.x * dUp);
                    bounds.AddPoint(p);
                }
            }
        }
    }

    fstart = dFar * start.z;
    fend = dFar * end.z;
    lstart = dUp * start.x;
    lend = dUp * end.x;

    // test up plane
    d1 = -fstart + lstart;
    d2 = -fend + lend;
    cull1 |= IEEE_FLT_SIGNBITSET(d1) << 2;
    cull2 |= IEEE_FLT_SIGNBITSET(d2) << 2;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (p.x > 0.0f) {
                p.y = start.y + f * dir.y;
                if (Math::Fabs(p.y) <= p.x * leftScale) {
                    p.y = p.y * dFar / (p.x * dLeft);
                    p.z = 1.0f;
                    bounds.AddPoint(p);
                }
            }
        }
    }

    // test down plane
    d1 = fstart + lstart;
    d2 = fend + lend;
    cull1 |= IEEE_FLT_SIGNBITSET(d1) << 3;
    cull2 |= IEEE_FLT_SIGNBITSET(d2) << 3;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (p.x > 0.0f) {
                p.y = start.y + f * dir.y;
                if (Math::Fabs(p.y) <= p.x * leftScale) {
                    p.y = p.y * dFar / (p.x * dLeft);
                    p.z = -1.0f;
                    bounds.AddPoint(p);
                }
            }
        }
    }

    if (cull1 == 0 && start.x > 0.0f) {
        // add start point to projection bounds
        p.x = start.x;
        p.y = start.y * dFar / (start.x * dLeft);
        p.z = start.z * dFar / (start.x * dUp);
        bounds.AddPoint(p);
    }

    if (cull2 == 0 && end.x > 0.0f) {
        // add end point to projection bounds
        p.x = end.x;
        p.y = end.y * dFar / (end.x * dLeft);
        p.z = end.z * dFar / (end.x * dUp);
        bounds.AddPoint(p);
    }

    if (start.x < bounds[0].x) {
        bounds[0].x = start.x < 0.0f ? 0.0f : start.x;
    }

    if (end.x < bounds[0].x) {
        bounds[0].x = end.x < 0.0f ? 0.0f : end.x;
    }

    startCull = cull1;
    endCull = cull2;
}

void Frustum::AddLocalLineToProjectionBoundsUseCull(const Vec3 &start, const Vec3 &end, int startCull, int endCull, AABB &bounds) const {
    Vec3 p;
    float d1, d2, fstart, fend, lstart, lend, f;

    int clip = startCull ^ endCull;
    if (!clip) {
        return;
    }

    float leftScale = dLeft * invFar;
    float upScale = dUp * invFar;
    Vec3 dir = end - start;

    if (clip & (1|2)) {
        fstart = dFar * start.y;
        fend = dFar * end.y;
        lstart = dLeft * start.x;
        lend = dLeft * end.x;

        if (clip & 1) {
            // test left plane
            d1 = -fstart + lstart;
            d2 = -fend + lend;
            if (IEEE_FLT_NOTZERO(d1)) {
                if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
                    f = d1 / (d1 - d2);
                    p.x = start.x + f * dir.x;
                    if (p.x > 0.0f) {
                        p.z = start.z + f * dir.z;
                        if (Math::Fabs(p.z) <= p.x * upScale) {
                            p.y = 1.0f;
                            p.z = p.z * dFar / (p.x * dUp);
                            bounds.AddPoint(p);
                        }
                    }
                }
            }
        }

        if (clip & 2) {
            // test right plane
            d1 = fstart + lstart;
            d2 = fend + lend;
            if (IEEE_FLT_NOTZERO(d1)) {
                if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
                    f = d1 / (d1 - d2);
                    p.x = start.x + f * dir.x;
                    if (p.x > 0.0f) {
                        p.z = start.z + f * dir.z;
                        if (Math::Fabs(p.z ) <= p.x * upScale) {
                            p.y = -1.0f;
                            p.z = p.z * dFar / (p.x * dUp);
                            bounds.AddPoint(p);
                        }
                    }
                }
            }
        }
    }

    if (clip & (4|8)) {
        fstart = dFar * start.z;
        fend = dFar * end.z;
        lstart = dUp * start.x;
        lend = dUp * end.x;

        if (clip & 4) {
            // test up plane
            d1 = -fstart + lstart;
            d2 = -fend + lend;
            if (IEEE_FLT_NOTZERO(d1)) {
                if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
                    f = d1 / (d1 - d2);
                    p.x = start.x + f * dir.x;
                    if (p.x > 0.0f) {
                        p.y = start.y + f * dir.y;
                        if (Math::Fabs(p.y) <= p.x * leftScale) {
                            p.y = p.y * dFar / (p.x * dLeft);
                            p.z = 1.0f;
                            bounds.AddPoint(p);
                        }
                    }
                }
            }
        }

        if (clip & 8) {
            // test down plane
            d1 = fstart + lstart;
            d2 = fend + lend;
            if (IEEE_FLT_NOTZERO(d1)) {
                if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
                    f = d1 / (d1 - d2);
                    p.x = start.x + f * dir.x;
                    if (p.x > 0.0f) {
                        p.y = start.y + f * dir.y;
                        if (Math::Fabs(p.y) <= p.x * leftScale) {
                            p.y = p.y * dFar / (p.x * dLeft);
                            p.z = -1.0f;
                            bounds.AddPoint(p);
                        }
                    }
                }
            }
        }
    }
}

// Returns true if the ray starts inside the bounds.
// If there was an intersection scale1 <= scale2
bool Frustum::AABBRayIntersection(const AABB &bounds, const Vec3 &start, const Vec3 &dir, float &scale1, float &scale2) const {
    Vec3 p;
    float d1, d2, f;
    int startInside = 1;

    scale1 = Math::Infinity;
    scale2 = -Math::Infinity;

    Vec3 end = start + dir;

    for (int i = 0; i < 2; i++) {
        d1 = start.x - bounds[i].x;
        startInside &= IEEE_FLT_SIGNBITSET(d1) ^ i;
        d2 = end.x - bounds[i].x;
        if (d1 != d2) {
            f = d1 / (d1 - d2);
            p.y = start.y + f * dir.y;
            if (bounds[0].y <= p.y && p.y <= bounds[1].y) {
                p.z = start.z + f * dir.z;
                if (bounds[0].z <= p.z && p.z <= bounds[1].z) {
                    if (f < scale1) scale1 = f;
                    if (f > scale2) scale2 = f;
                }
            }
        }

        d1 = start.y - bounds[i].y;
        startInside &= IEEE_FLT_SIGNBITSET(d1) ^ i;
        d2 = end.y - bounds[i].y;
        if (d1 != d2) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (bounds[0].x <= p.x && p.x <= bounds[1].x) {
                p.z = start.z + f * dir.z;
                if (bounds[0].z <= p.z && p.z <= bounds[1].z) {
                    if (f < scale1) scale1 = f;
                    if (f > scale2) scale2 = f;
                }
            }
        }

        d1 = start.z - bounds[i].z;
        startInside &= IEEE_FLT_SIGNBITSET(d1) ^ i;
        d2 = end.z - bounds[i].z;
        if (d1 != d2) {
            f = d1 / (d1 - d2);
            p.x = start.x + f * dir.x;
            if (bounds[0].x <= p.x && p.x <= bounds[1].x) {
                p.y = start.y + f * dir.y;
                if (bounds[0].y <= p.y && p.y <= bounds[1].y) {
                    if (f < scale1) scale1 = f;
                    if (f > scale2) scale2 = f;
                }
            }
        }
    }

    return (startInside != 0);
}

bool Frustum::ProjectionBounds(const AABB &bounds, AABB &projectionBounds) const {
    return ProjectionBounds(OBB(bounds, Vec3::origin, Mat3::identity), projectionBounds);
}

//#if !defined(__UNIX__)
bool Frustum::ProjectionBounds(const OBB &box, AABB &projectionBounds) const {
    int p1, p2, pointCull[8];
    float scale1, scale2;
    Vec3 points[8];
    AABB bounds(-box.Extents(), box.Extents());

    // if the frustum origin is inside the bounds
    if (bounds.IsContainPoint(box.Axis().TransposedMulVec(origin - box.Center()))) {
        // bounds that cover the whole frustum
        float boxMin, boxMax, base;

        base = origin.Dot(axis[0]);
        box.ProjectOnAxis(axis[0], boxMin, boxMax);

        projectionBounds[0].x = boxMin - base;
        projectionBounds[1].x = boxMax - base;
        projectionBounds[0].y = projectionBounds[0].z = -1.0f;
        projectionBounds[1].y = projectionBounds[1].z = 1.0f;
        return true;
    }

    projectionBounds.Clear();

    // transform the bounds into the space of this frustum
    Vec3 localOrigin = axis.TransposedMulVec(box.Center() - origin);
    Mat3 localAxis = axis.TransposedMul(box.Axis());
    BoxToPoints(localOrigin, box.Extents(), localAxis, points);

    // test outer four edges of the bounds
    int culled = -1;
    int outside = 0;
    for (int i = 0; i < 4; i++) {
        p1 = i;
        p2 = 4 + i;
        AddLocalLineToProjectionBoundsSetCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
        culled &= pointCull[p1] & pointCull[p2];
        outside |= pointCull[p1] | pointCull[p2];
    }

    // if the bounds are completely outside this frustum
    if (culled) {
        return false;
    }

    // if the bounds are completely inside this frustum
    if (!outside) {
        return true;
    }

    // test the remaining edges of the bounds
    for (int i = 0; i < 4; i++) {
        p1 = i;
        p2 = (i+1)&3;
        AddLocalLineToProjectionBoundsUseCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
    }

    for (int i = 0; i < 4; i++) {
        p1 = 4 + i;
        p2 = 4 + ((i+1)&3);
        AddLocalLineToProjectionBoundsUseCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
    }

    // if the bounds extend beyond two or more boundaries of this frustum
    if (outside != 1 && outside != 2 && outside != 4 && outside != 8) {
        localOrigin = box.Axis().TransposedMulVec(origin - box.Center());
        Mat3 localScaled = box.Axis().TransposedMul(axis);
        localScaled[0] *= dFar;
        localScaled[1] *= dLeft;
        localScaled[2] *= dUp;

        // test the outer edges of this frustum for intersection with the bounds
        if ((outside & 2) && (outside & 8)) {
            AABBRayIntersection(bounds, localOrigin, localScaled[0] - localScaled[1] - localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, -1.0f, -1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, -1.0f, -1.0f));
            }
        }

        if ((outside & 2) && (outside & 4)) {
            AABBRayIntersection(bounds, localOrigin, localScaled[0] - localScaled[1] + localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, -1.0f, 1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, -1.0f, 1.0f));
            }
        }

        if ((outside & 1) && (outside & 8)) {
            AABBRayIntersection(bounds, localOrigin, localScaled[0] + localScaled[1] - localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, 1.0f, -1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, 1.0f, -1.0f));
            }
        }

        if ((outside & 1) && (outside & 2)) {
            AABBRayIntersection(bounds, localOrigin, localScaled[0] + localScaled[1] + localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, 1.0f, 1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, 1.0f, 1.0f));
            }
        }
    }

    return true;
}
//#endif

bool Frustum::ProjectionBounds(const Sphere &sphere, AABB &projectionBounds) const {
    projectionBounds.Clear();

    Vec3 center = axis.TransposedMulVec(sphere.Center() - origin);
    float r = sphere.Radius();
    float rs = r * r;
    float sFar = dFar * dFar;

    // test left/right planes
    float d = dFar * Math::Fabs(center.y) - dLeft * center.x;
    if ((d * d) > rs * (sFar + dLeft * dLeft)) {
        return false;
    }
    
    // test up/down planes
    d = dFar * Math::Fabs(center.z) - dUp * center.x;
    if ((d * d) > rs * (sFar + dUp * dUp)) {
        return false;
    }
    
    // FIXME: implement

    // bounds that cover the whole frustum
    projectionBounds[0].x = 0.0f;
    projectionBounds[1].x = dFar;
    projectionBounds[0].y = projectionBounds[0].z = -1.0f;
    projectionBounds[1].y = projectionBounds[1].z = 1.0f;
    return true;
}

bool Frustum::ProjectionBounds(const Frustum &frustum, AABB &projectionBounds) const {
    int p1, p2, pointCull[8];
    float scale1, scale2;
    Vec3 points[8], localOrigin;
    Mat3 localScaled;

    // if the frustum origin is inside the other frustum
    if (frustum.IsContainPoint(origin)) {
        // bounds that cover the whole frustum
        float frustumMin, frustumMax;
        float base = origin.Dot(axis[0]);
        frustum.ProjectOnAxis(axis[0], frustumMin, frustumMax);

        projectionBounds[0].x = frustumMin - base;
        projectionBounds[1].x = frustumMax - base;
        projectionBounds[0].y = projectionBounds[0].z = -1.0f;
        projectionBounds[1].y = projectionBounds[1].z = 1.0f;
        return true;
    }

    projectionBounds.Clear();

    // transform the given frustum into the space of this frustum
    Frustum localFrustum = frustum;
    localFrustum.origin = axis.TransposedMulVec(frustum.origin - origin);
    localFrustum.axis = axis.TransposedMul(frustum.axis);
    localFrustum.ToPoints(points);

    // test outer four edges of the other frustum
    int culled = -1;
    int outside = 0;
    for (int i = 0; i < 4; i++) {
        p1 = i;
        p2 = 4 + i;
        AddLocalLineToProjectionBoundsSetCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
        culled &= pointCull[p1] & pointCull[p2];
        outside |= pointCull[p1] | pointCull[p2];
    }

    // if the other frustum is completely outside this frustum
    if (culled) {
        return false;
    }
    
    // if the other frustum is completely inside this frustum
    if (!outside) {
        return true;
    }
    
    // test the remaining edges of the other frustum
    if (localFrustum.dNear > 0.0f) {
        for (int i = 0; i < 4; i++) {
            p1 = i;
            p2 = (i+1)&3;
            AddLocalLineToProjectionBoundsUseCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
        }
    }

    for (int i = 0; i < 4; i++) {
        p1 = 4 + i;
        p2 = 4 + ((i+1)&3);
        AddLocalLineToProjectionBoundsUseCull(points[p1], points[p2], pointCull[p1], pointCull[p2], projectionBounds);
    }

    // if the other frustum extends beyond two or more boundaries of this frustum
    if (outside != 1 && outside != 2 && outside != 4 && outside != 8) {
        localOrigin = frustum.axis.TransposedMulVec(origin - frustum.origin);
        localScaled = frustum.axis.TransposedMul(axis);
        localScaled[0] *= dFar;
        localScaled[1] *= dLeft;
        localScaled[2] *= dUp;

        // test the outer edges of this frustum for intersection with the other frustum
        if ((outside & 2) && (outside & 8)) {
            frustum.LocalRayIntersection(localOrigin, localScaled[0] - localScaled[1] - localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, -1.0f, -1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, -1.0f, -1.0f));
            }
        }

        if ((outside & 2) && (outside & 4)) {
            frustum.LocalRayIntersection(localOrigin, localScaled[0] - localScaled[1] + localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, -1.0f, 1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, -1.0f, 1.0f));
            }
        }

        if ((outside & 1) && (outside & 8)) {
            frustum.LocalRayIntersection(localOrigin, localScaled[0] + localScaled[1] - localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, 1.0f, -1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, 1.0f, -1.0f));
            }
        }

        if ((outside & 1) && (outside & 2)) {
            frustum.LocalRayIntersection(localOrigin, localScaled[0] + localScaled[1] + localScaled[2], scale1, scale2);
            if (scale1 <= scale2 && scale1 >= 0.0f) {
                projectionBounds.AddPoint(Vec3(scale1 * dFar, 1.0f, 1.0f));
                projectionBounds.AddPoint(Vec3(scale2 * dFar, 1.0f, 1.0f));
            }
        }
    }

    return true;
}

// Clips the frustum far extents to the box.
void Frustum::ClipFrustumToOBB(const OBB &box, float clipFractions[4], int clipPlanes[4]) const {
    int index;
    float f;

    Vec3 localOrigin = box.Axis().TransposedMulVec(origin - box.Center());
    Mat3 localAxis = box.Axis().TransposedMul(axis);

    Mat3 scaled;
    scaled[0] = localAxis[0] * dFar;
    scaled[1] = localAxis[1] * dLeft;
    scaled[2] = localAxis[2] * dUp;

    Vec3 cornerVecs[4];
    cornerVecs[0] = scaled[0] + scaled[1];
    cornerVecs[1] = scaled[0] - scaled[1];
    cornerVecs[2] = cornerVecs[1] - scaled[2];
    cornerVecs[3] = cornerVecs[0] - scaled[2];
    cornerVecs[0] += scaled[2];
    cornerVecs[1] += scaled[2];

    AABB bounds;
    bounds[0] = -box.Extents();
    bounds[1] = box.Extents();

    float minf = (dNear + 1.0f) * invFar;

    for (int i = 0; i < 4; i++) {
        index = IEEE_FLT_SIGNBITNOTSET(cornerVecs[i].x);
        f = (bounds[index].x - localOrigin.x) / cornerVecs[i].x;
        clipFractions[i] = f;
        clipPlanes[i] = 1 << index;

        index = IEEE_FLT_SIGNBITNOTSET(cornerVecs[i].y);
        f = (bounds[index].y - localOrigin.y) / cornerVecs[i].y;
        if (f < clipFractions[i]) {
            clipFractions[i] = f;
            clipPlanes[i] = 4 << index;
        }

        index = IEEE_FLT_SIGNBITNOTSET(cornerVecs[i].z);
        f = (bounds[index].z - localOrigin.z) / cornerVecs[i].z;
        if (f < clipFractions[i]) {
            clipFractions[i] = f;
            clipPlanes[i] = 16 << index;
        }

        // make sure the frustum is not clipped between the frustum origin and the near plane
        if (clipFractions[i] < minf) {
            clipFractions[i] = minf;
        }
    }
}

// Returns true if part of the line is inside the frustum.
// Does not clip to the near and far plane.
bool Frustum::ClipLine(const Vec3 localPoints[8], const Vec3 points[8], int startIndex, int endIndex, Vec3 &start, Vec3 &end, int &startClip, int &endClip) const {
    float f, x;

    float leftScale = dLeft * invFar;
    float upScale = dUp * invFar;

    Vec3 localStart = localPoints[startIndex];
    Vec3 localEnd = localPoints[endIndex];
    Vec3 localDir = localEnd - localStart;

    startClip = endClip = -1;
    float scale1 = Math::Infinity;
    float scale2 = -Math::Infinity;

    float fstart = dFar * localStart.y;
    float fend = dFar * localEnd.y;
    float lstart = dLeft * localStart.x;
    float lend = dLeft * localEnd.x;

    // test left plane
    float d1 = -fstart + lstart;
    float d2 = -fend + lend;
    int startCull = IEEE_FLT_SIGNBITSET(d1);
    int endCull = IEEE_FLT_SIGNBITSET(d2);
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = localStart.x + f * localDir.x;
            if (x >= 0.0f) {
                if (Math::Fabs(localStart.z + f * localDir.z) <= x * upScale) {
                    if (f < scale1) { scale1 = f; startClip = 0; }
                    if (f > scale2) { scale2 = f; endClip = 0; }
                }
            }
        }
    }

    // test right plane
    d1 = fstart + lstart;
    d2 = fend + lend;
    startCull |= IEEE_FLT_SIGNBITSET(d1) << 1;
    endCull |= IEEE_FLT_SIGNBITSET(d2) << 1;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = localStart.x + f * localDir.x;
            if (x >= 0.0f) {
                if (Math::Fabs(localStart.z + f * localDir.z) <= x * upScale) {
                    if (f < scale1) { scale1 = f; startClip = 1; }
                    if (f > scale2) { scale2 = f; endClip = 1; }
                }
            }
        }
    }

    fstart = dFar * localStart.z;
    fend = dFar * localEnd.z;
    lstart = dUp * localStart.x;
    lend = dUp * localEnd.x;

    // test up plane
    d1 = -fstart + lstart;
    d2 = -fend + lend;
    startCull |= IEEE_FLT_SIGNBITSET(d1) << 2;
    endCull |= IEEE_FLT_SIGNBITSET(d2) << 2;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = localStart.x + f * localDir.x;
            if (x >= 0.0f) {
                if (Math::Fabs(localStart.y + f * localDir.y) <= x * leftScale) {
                    if (f < scale1) { scale1 = f; startClip = 2; }
                    if (f > scale2) { scale2 = f; endClip = 2; }
                }
            }
        }
    }

    // test down plane
    d1 = fstart + lstart;
    d2 = fend + lend;
    startCull |= IEEE_FLT_SIGNBITSET(d1) << 3;
    endCull |= IEEE_FLT_SIGNBITSET(d2) << 3;
    if (IEEE_FLT_NOTZERO(d1)) {
        if (IEEE_FLT_SIGNBITSET(d1) ^ IEEE_FLT_SIGNBITSET(d2)) {
            f = d1 / (d1 - d2);
            x = localStart.x + f * localDir.x;
            if (x >= 0.0f) {
                if (Math::Fabs(localStart.y + f * localDir.y) <= x * leftScale) {
                    if (f < scale1) { scale1 = f; startClip = 3; }
                    if (f > scale2) { scale2 = f; endClip = 3; }
                }
            }
        }
    }

    // if completely inside
    if (!(startCull | endCull)) {
        start = points[startIndex];
        end = points[endIndex];
        return true;
    } else if (scale1 <= scale2) {
        if (!startCull) {
            start = points[startIndex];
            startClip = -1;
        } else {
            start = points[startIndex] + scale1 * (points[endIndex] - points[startIndex]);
        }

        if (!endCull) {
            end = points[endIndex];
            endClip = -1;
        } else {
            end = points[startIndex] + scale2 * (points[endIndex] - points[startIndex]);
        }
        return true;
    }
    return false;
}

static int capPointIndex[4][2] = {
    { 0, 3 },
    { 1, 2 },
    { 0, 1 },
    { 2, 3 }
};

BE_INLINE bool Frustum::AddLocalCapsToProjectionBounds(const Vec3 endPoints[4], const int endPointCull[4], const Vec3 &point, int pointCull, int pointClip, AABB &projectionBounds) const {
    if (pointClip < 0) {
        return false;
    }
    
    int *p = capPointIndex[pointClip];
    AddLocalLineToProjectionBoundsUseCull(endPoints[p[0]], point, endPointCull[p[0]], pointCull, projectionBounds);
    AddLocalLineToProjectionBoundsUseCull(endPoints[p[1]], point, endPointCull[p[1]], pointCull, projectionBounds);
    return true;
}

bool Frustum::ClippedProjectionBounds(const Frustum &frustum, const OBB &clipBox, AABB &projectionBounds) const {
    int p1, p2, clipPointCull[8], clipPlanes[4];
    int pointCull[2], startClip, endClip, boxPointCull[8];
    float clipFractions[4], s1, s2, t1, t2;
    Vec3 clipPoints[8], localPoints1[8], localPoints2[8], localOrigin1, localOrigin2, start, end;
    Mat3 localAxis1, localAxis2;
    AABB clipBounds;

    // if the frustum origin is inside the other frustum
    if (frustum.IsContainPoint(origin)) {
        // bounds that cover the whole frustum
        float clipBoxMin, clipBoxMax, frustumMin, frustumMax, base;

        base = origin.Dot(axis[0]);
        clipBox.ProjectOnAxis(axis[0], clipBoxMin, clipBoxMax);
        frustum.ProjectOnAxis(axis[0], frustumMin, frustumMax);

        projectionBounds[0].x = Max(clipBoxMin, frustumMin) - base;
        projectionBounds[1].x = Min(clipBoxMax, frustumMax) - base;
        projectionBounds[0].y = projectionBounds[0].z = -1.0f;
        projectionBounds[1].y = projectionBounds[1].z = 1.0f;
        return true;
    }

    projectionBounds.Clear();

    // clip the outer edges of the given frustum to the clip bounds
    frustum.ClipFrustumToOBB(clipBox, clipFractions, clipPlanes);
    int usedClipPlanes = clipPlanes[0] | clipPlanes[1] | clipPlanes[2] | clipPlanes[3];

    // transform the clipped frustum to the space of this frustum
    Frustum localFrustum = frustum;
    localFrustum.origin = axis.TransposedMulVec(frustum.origin - origin);
    localFrustum.axis = axis.TransposedMul(frustum.axis);
    localFrustum.ToClippedPoints(clipFractions, clipPoints);

    // test outer four edges of the clipped frustum
    for (int i = 0; i < 4; i++) {
        p1 = i;
        p2 = 4 + i;
        AddLocalLineToProjectionBoundsSetCull(clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds);
    }

    // get cull bits for the clipped frustum
    int outside = clipPointCull[0] | clipPointCull[1] | clipPointCull[2] | clipPointCull[3] | clipPointCull[4] | clipPointCull[5] | clipPointCull[6] | clipPointCull[7];
    int nearCull = clipPointCull[0] & clipPointCull[1] & clipPointCull[2] & clipPointCull[3];
    int farCull = clipPointCull[4] & clipPointCull[5] & clipPointCull[6] & clipPointCull[7];

    // if the clipped frustum is not completely inside this frustum
    if (outside) {
        // test the remaining edges of the clipped frustum
        if (!nearCull && localFrustum.dNear > 0.0f) {
            for (int i = 0; i < 4; i++) {
                p1 = i;
                p2 = (i+1)&3;
                AddLocalLineToProjectionBoundsUseCull(clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds);
            }
        }

        if (!farCull) {
            for (int i = 0; i < 4; i++) {
                p1 = 4 + i;
                p2 = 4 + ((i+1)&3);
                AddLocalLineToProjectionBoundsUseCull(clipPoints[p1], clipPoints[p2], clipPointCull[p1], clipPointCull[p2], projectionBounds);
            }
        }
    }

    // if the clipped frustum far end points are inside this frustum
    if (!(farCull && !(nearCull & farCull)) &&
        // if the clipped frustum is not clipped to a single plane of the clip bounds
        (clipPlanes[0] != clipPlanes[1] || clipPlanes[1] != clipPlanes[2] || clipPlanes[2] != clipPlanes[3])) {
        // transform the clip box into the space of the other frustum
        localOrigin1 = frustum.axis.TransposedMulVec(clipBox.Center() - frustum.origin);
        localAxis1 = frustum.axis.TransposedMul(clipBox.Axis());
        BoxToPoints(localOrigin1, clipBox.Extents(), localAxis1, localPoints1);

        // cull the box corners with the other frustum
        float leftScale = frustum.dLeft * frustum.invFar;
        float upScale = frustum.dUp * frustum.invFar;
        for (int i = 0; i < 8; i++) {
            Vec3 &p = localPoints1[i];
            if (!(boxVertPlanes[i] & usedClipPlanes) || p.x <= 0.0f) {
                boxPointCull[i] = 1|2|4|8;
            } else {
                boxPointCull[i] = 0;
                if (Math::Fabs(p.y) > p.x * leftScale) {
                    boxPointCull[i] |= 1 << IEEE_FLT_SIGNBITSET(p.y);
                }
                if (Math::Fabs(p.z) > p.x * upScale) {
                    boxPointCull[i] |= 4 << IEEE_FLT_SIGNBITSET(p.z);
                }
            }
        }

        // transform the clip box into the space of this frustum
        localOrigin2 = axis.TransposedMulVec(clipBox.Center() - origin);
        localAxis2 = axis.TransposedMul(clipBox.Axis());
        BoxToPoints(localOrigin2, clipBox.Extents(), localAxis2, localPoints2);

        // clip the edges of the clip bounds to the other frustum and add the clipped edges to the projection bounds
        for (int i = 0; i < 4; i++) {
            p1 = i;
            p2 = 4 + i;
            if (!(boxPointCull[p1] & boxPointCull[p2])) {
                if (frustum.ClipLine(localPoints1, localPoints2, p1, p2, start, end, startClip, endClip)) {
                    AddLocalLineToProjectionBoundsSetCull(start, end, pointCull[0], pointCull[1], projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds);
                    outside |= pointCull[0] | pointCull[1];
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            p1 = i;
            p2 = (i+1)&3;
            if (!(boxPointCull[p1] & boxPointCull[p2])) {
                if (frustum.ClipLine(localPoints1, localPoints2, p1, p2, start, end, startClip, endClip)) {
                    AddLocalLineToProjectionBoundsSetCull(start, end, pointCull[0], pointCull[1], projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds);
                    outside |= pointCull[0] | pointCull[1];
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            p1 = 4 + i;
            p2 = 4 + ((i+1)&3);
            if (!(boxPointCull[p1] & boxPointCull[p2])) {
                if (frustum.ClipLine(localPoints1, localPoints2, p1, p2, start, end, startClip, endClip)) {
                    AddLocalLineToProjectionBoundsSetCull(start, end, pointCull[0], pointCull[1], projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, start, pointCull[0], startClip, projectionBounds);
                    AddLocalCapsToProjectionBounds(clipPoints+4, clipPointCull+4, end, pointCull[1], endClip, projectionBounds);
                    outside |= pointCull[0] | pointCull[1];
                }
            }
        }
    }

    // if the clipped frustum extends beyond two or more boundaries of this frustum
    if (outside != 1 && outside != 2 && outside != 4 && outside != 8) {
        // transform this frustum into the space of the other frustum
        localOrigin1 = frustum.axis.TransposedMulVec(origin - frustum.origin);
        localAxis1 = frustum.axis.TransposedMul(axis);
        localAxis1[0] *= dFar;
        localAxis1[1] *= dLeft;
        localAxis1[2] *= dUp;

        // transform this frustum into the space of the clip bounds
        localOrigin2 = clipBox.Axis().TransposedMulVec(origin - clipBox.Center());
        localAxis2 = clipBox.Axis().TransposedMul(axis);
        localAxis2[0] *= dFar;
        localAxis2[1] *= dLeft;
        localAxis2[2] *= dUp;

        clipBounds[0] = -clipBox.Extents();
        clipBounds[1] = clipBox.Extents();

        // test the outer edges of this frustum for intersection with both the other frustum and the clip bounds
        if ((outside & 2) && (outside & 8)) {
            frustum.LocalRayIntersection(localOrigin1, localAxis1[0] - localAxis1[1] - localAxis1[2], s1, s2);
            if (s1 <= s2 && s1 >= 0.0f) {
                AABBRayIntersection(clipBounds, localOrigin2, localAxis2[0] - localAxis2[1] - localAxis2[2], t1, t2);
                if (t1 <= t2 && t2 > s1 && t1 < s2) {
                    projectionBounds.AddPoint(Vec3(s1 * dFar, -1.0f, -1.0f));
                    projectionBounds.AddPoint(Vec3(s2 * dFar, -1.0f, -1.0f));
                }
            }
        }

        if ((outside & 2) && (outside & 4)) {
            frustum.LocalRayIntersection(localOrigin1, localAxis1[0] - localAxis1[1] + localAxis1[2], s1, s2);
            if (s1 <= s2 && s1 >= 0.0f) {
                AABBRayIntersection(clipBounds, localOrigin2, localAxis2[0] - localAxis2[1] + localAxis2[2], t1, t2);
                if (t1 <= t2 && t2 > s1 && t1 < s2) {
                    projectionBounds.AddPoint(Vec3(s1 * dFar, -1.0f, 1.0f));
                    projectionBounds.AddPoint(Vec3(s2 * dFar, -1.0f, 1.0f));
                }
            }
        }

        if ((outside & 1) && (outside & 8)) {
            frustum.LocalRayIntersection(localOrigin1, localAxis1[0] + localAxis1[1] - localAxis1[2], s1, s2);
            if (s1 <= s2 && s1 >= 0.0f) {
                AABBRayIntersection(clipBounds, localOrigin2, localAxis2[0] + localAxis2[1] - localAxis2[2], t1, t2);
                if (t1 <= t2 && t2 > s1 && t1 < s2) {
                    projectionBounds.AddPoint(Vec3(s1 * dFar, 1.0f, -1.0f));
                    projectionBounds.AddPoint(Vec3(s2 * dFar, 1.0f, -1.0f));
                }
            }
        }

        if ((outside & 1) && (outside & 2)) {
            frustum.LocalRayIntersection(localOrigin1, localAxis1[0] + localAxis1[1] + localAxis1[2], s1, s2);
            if (s1 <= s2 && s1 >= 0.0f) {
                AABBRayIntersection(clipBounds, localOrigin2, localAxis2[0] + localAxis2[1] + localAxis2[2], t1, t2);
                if (t1 <= t2 && t2 > s1 && t1 < s2) {
                    projectionBounds.AddPoint(Vec3(s1 * dFar, 1.0f, 1.0f));
                    projectionBounds.AddPoint(Vec3(s2 * dFar, 1.0f, 1.0f));
                }
            }
        }
    }

    return true;
}

BE_NAMESPACE_END
