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

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

struct Coords2D {
    BE_FORCE_INLINE static int XAxisIndex() {
        return 1;
    }

    BE_FORCE_INLINE static int YAxisIndex() {
        return 2;
    }

    BE_FORCE_INLINE static int ZAxisIndex() {
        return 0;
    }

    BE_FORCE_INLINE static Vec3 XAxis() {
        return Vec3::unitY;
    }

    BE_FORCE_INLINE static Vec3 YAxis() {
        return Vec3::unitZ;
    }

    BE_FORCE_INLINE static Vec3 ZAxis() {
        return Vec3::unitX;
    }

    BE_FORCE_INLINE static Vec3 To3D(const Vec2 &rectPos, float rectZ = 0.0f) {
        return Vec3(rectZ, rectPos);
    }

    BE_FORCE_INLINE static Vec3 To3D(float rectX, float rectY, float rectZ = 0.0f) {
        return Vec3(rectZ, rectX, rectY);
    }

    BE_FORCE_INLINE static Vec2 From3D(const Vec3 &worldPos) {
        return worldPos.yz();
    }

    BE_FORCE_INLINE static float ZFrom3D(const Vec3 &worldPos) {
        return worldPos.x;
    }
};

BE_NAMESPACE_END
