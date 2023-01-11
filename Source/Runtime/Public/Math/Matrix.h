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

    3x3 matrices are column major order,
    all other matrices are row major order.

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
#include "Matrix3x4.h"
#include "Matrix4.h"

BE_NAMESPACE_BEGIN

template <typename Matrix>
BE_INLINE bool IsMatrixSymmetric(const Matrix &mat, const float epsilon = MATRIX_EPSILON) {
    for (int i = 1; i < Matrix::Rows; i++) {
        for (int j = 0; j < i; j++) {
            if (Math::Fabs(mat[i][j] - mat[j][i]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

template <typename Matrix>
BE_INLINE bool IsMatrixDiagonal(const Matrix &mat, const float epsilon = MATRIX_EPSILON) {
    for (int i = 0; i < Matrix::Rows; i++) {
        for (int j = 0; j < Matrix::Cols; j++) {
            if (i != j && Math::Fabs(mat[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

/// LU decomposition, in-place.
/// Doolittle Algorithm : uses unit diagonals for the lower triangle.
template <typename Matrix>
bool DecompLUDoolittle(Matrix &mat, const float epsilon = MATRIX_EPSILON) {
    float sum;

    for (int i = 0; i < Matrix::Rows; i++) {
        // Compute a line of U
        for (int j = i; j < Matrix::Cols; j++) {
            sum = 0;
            for (int k = 0; k < i; k++) {
                sum += mat[i][k] * mat[k][j];
            }
            mat[i][j] = mat[i][j] - sum; // not dividing by diagonals
        }
        if (Math::Fabs(mat[i][i]) <= epsilon) {
            return false;
        }
        // Compute a line of L
        for (int j = 0; j < i; j++) {
            sum = 0;
            for (int k = 0; k < j; k++) {
                sum += mat[i][k] * mat[k][j];
            }
            mat[i][j] = (mat[i][j] - sum) / mat[j][j];
        }
    }
    return true;
}

/// Solve Ax = b with Doolittle decomposed LU matrix.
template <typename Matrix, typename Vector>
Vector SolveLUDoolittle(const Matrix &mat, const Vector &b) {
    static_assert(Matrix::Rows == Vector::Size);
    static_assert(Matrix::Cols == Vector::Size);

    Vector x, y;
    float sum;

    // Solve Ly = b
    for (int i = 0; i < Matrix::Rows; i++) {
        sum = 0;
        for (int j = 0; j < i; j++) {
            sum += mat[i][j] * y[j];
        }
        y[i] = b[i] - sum; // not dividing by diagonals
    }
    // Solve Ux = y
    for (int i = Matrix::Rows - 1; i >= 0; i--) {
        sum = 0;
        for (int j = i; j < Matrix::Cols; j++) {
            sum += mat[i][j] * x[j];
        }
        x[i] = (y[i] - sum) / mat[i][i];
    }
    return x;
}

BE_NAMESPACE_END
