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

    Matrix Classes

    3x3 행렬은 column major order, 그 외 모든 행렬은 row major order

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

#define MATRIX_INVERSE_EPSILON      1e-14
#define MATRIX_EPSILON              1e-6

class Mat2;
class Mat3;
class Mat3x4;
class Mat4;
class Angles;
class Rotation;
class Quat;
class Plane;

BE_NAMESPACE_END

#include "Matrix2.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Matrix3x4.h"
