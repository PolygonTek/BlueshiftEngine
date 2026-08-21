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

#pragma once

BE_NAMESPACE_BEGIN

namespace JacobiRotation {

// Compact representation of a Jacobi plane rotation. The tangent is retained so
// a symmetric similarity transform can update its diagonal pair without recomputing it.
struct Coefficients {
    float c;
    float s;
    float t;
};

// Computes the smaller-magnitude rotation that diagonalizes a symmetric 2x2 block.
static BE_FORCE_INLINE Coefficients ComputeSymmetric(float diagonalP, float diagonalQ, float offDiagonal) {
    if (offDiagonal == 0.0f) {
        return { 1.0f, 0.0f, 0.0f };
    }

    // The half-scaled numerator avoids overflowing the equivalent
    // (diagonalQ - diagonalP) / (2 * offDiagonal) expression.
    float tau = (0.5f * diagonalQ - 0.5f * diagonalP) / offDiagonal;
    float absTau = Math::Abs(tau);
    float sign = tau >= 0.0f ? 1.0f : -1.0f;
    float t;

    // Evaluate tan(theta) without squaring a potentially large tau.
    if (absTau > 1.0f) {
        float invTau = 1.0f / absTau;
        t = sign * invTau / (1.0f + Math::Sqrt(1.0f + invTau * invTau));
    } else {
        t = sign / (absTau + Math::Sqrt(1.0f + absTau * absTau));
    }

    float c = 1.0f / Math::Sqrt(1.0f + t * t);
    return { c, t * c, t };
}

// Post-multiplies columns p and q by the given plane rotation.
template <int N>
static BE_FORCE_INLINE void RotateColumns(float matrix[N][N], int p, int q, const Coefficients &rotation) {
    for (int row = 0; row < N; row++) {
        float columnP = matrix[row][p];
        float columnQ = matrix[row][q];
        matrix[row][p] = rotation.c * columnP - rotation.s * columnQ;
        matrix[row][q] = rotation.s * columnP + rotation.c * columnQ;
    }
}

// Applies A = J^T * A * J while explicitly preserving symmetry and making the
// eliminated p-q pair exact. matrix uses conventional row-major indexing.
template <int N>
static BE_FORCE_INLINE void RotateSymmetric(float matrix[N][N], int p, int q, const Coefficients &rotation) {
    float app = matrix[p][p];
    float aqq = matrix[q][q];
    float apq = matrix[p][q];

    for (int i = 0; i < N; i++) {
        if (i == p || i == q) {
            continue;
        }

        float aip = matrix[i][p];
        float aiq = matrix[i][q];
        matrix[i][p] = matrix[p][i] = rotation.c * aip - rotation.s * aiq;
        matrix[i][q] = matrix[q][i] = rotation.s * aip + rotation.c * aiq;
    }

    matrix[p][p] = app - rotation.t * apq;
    matrix[q][q] = aqq + rotation.t * apq;
    matrix[p][q] = matrix[q][p] = 0.0f;
}

} // namespace JacobiRotation

BE_NAMESPACE_END
