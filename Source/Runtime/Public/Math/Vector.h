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

/*
-------------------------------------------------------------------------------

    Vector Classes

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

#define VECTOR_EPSILON      0.001f

class Vec2;
class Vec3;
class Vec4;
class Color3;
class Color4;
class Mat3;
class Angles;

BE_NAMESPACE_END

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

BE_NAMESPACE_BEGIN

BE_FORCE_INLINE Vec2 BE_API UnitToCenti(const Vec2 &x) { return x * 1.0f; }
BE_FORCE_INLINE Vec2 BE_API UnitToMeter(const Vec2 &x) { return UnitToCenti(x) * 0.01f; }
BE_FORCE_INLINE Vec2 BE_API CentiToUnit(const Vec2 &x) { return x / UnitToCenti(1.0f); }
BE_FORCE_INLINE Vec2 BE_API MeterToUnit(const Vec2 &x) { return CentiToUnit(x * 100.0f); }

BE_FORCE_INLINE Vec3 BE_API UnitToCenti(const Vec3 &x) { return x * 1.0f; }
BE_FORCE_INLINE Vec3 BE_API UnitToMeter(const Vec3 &x) { return UnitToCenti(x) * 0.01f; }
BE_FORCE_INLINE Vec3 BE_API CentiToUnit(const Vec3 &x) { return x / UnitToCenti(1.0f); }
BE_FORCE_INLINE Vec3 BE_API MeterToUnit(const Vec3 &x) { return CentiToUnit(x * 100.0f); }

BE_FORCE_INLINE Vec4 BE_API UnitToCenti(const Vec4 &x) { return x * 1.0f; }
BE_FORCE_INLINE Vec4 BE_API UnitToMeter(const Vec4 &x) { return UnitToCenti(x) * 0.01f; }
BE_FORCE_INLINE Vec4 BE_API CentiToUnit(const Vec4 &x) { return x / UnitToCenti(1.0f); }
BE_FORCE_INLINE Vec4 BE_API MeterToUnit(const Vec4 &x) { return CentiToUnit(x * 100.0f); }

BE_NAMESPACE_END
