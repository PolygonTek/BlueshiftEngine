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

const Pluecker Pluecker::origin(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
const Pluecker Pluecker::zero(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

// pluecker coordinate for the intersection of two planes
bool Pluecker::SetFromPlanes(const Plane &p1, const Plane &p2) {
    p[0] = -(p1[2] * -p2[3] - p2[2] * -p1[3]);
    p[1] = -(p2[1] * -p1[3] - p1[1] * -p2[3]);
    p[2] = p1[1] * p2[2] - p2[1] * p1[2];

    p[3] = -(p1[0] * -p2[3] - p2[0] * -p1[3]);
    p[4] = p1[0] * p2[1] - p2[0] * p1[1];
    p[5] = p1[0] * p2[2] - p2[0] * p1[2];

    return (p[2] != 0.0f || p[5] != 0.0f || p[4] != 0.0f);
}

// calculates square of shortest distance between the two
// 3D lines represented by their pluecker coordinates
float Pluecker::Distance3DSqr(const Pluecker &a) const {
    Vec3 dir;

    dir[0] = -a.p[5] *  p[4] -  a.p[4] * -p[5];
    dir[1] =  a.p[4] *  p[2] -  a.p[2] *  p[4];
    dir[2] =  a.p[2] * -p[5] - -a.p[5] *  p[2];
    if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f) {
        return -1.0f;   // FIXME: implement for parallel lines
    }

    float d = a.p[4] * ( p[2]*dir[1] - -p[5]*dir[0]) +
              a.p[5] * ( p[2]*dir[2] -  p[4]*dir[0]) +
              a.p[2] * (-p[5]*dir[2] -  p[4]*dir[1]);
    float s = PermutedInnerProduct(a) / d;
    return dir.Dot(dir) * (s * s);
}

BE_NAMESPACE_END
