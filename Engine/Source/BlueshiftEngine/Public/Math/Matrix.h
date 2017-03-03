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
