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

namespace LU {

static BE_FORCE_INLINE float GetMatrixElement(const Mat2 &matrix, int row, int column) {
    return matrix[row][column];
}

static BE_FORCE_INLINE float GetMatrixElement(const Mat3 &matrix, int row, int column) {
    return matrix[column][row];
}

static BE_FORCE_INLINE float GetMatrixElement(const Mat4 &matrix, int row, int column) {
    return matrix[row][column];
}

template <int N>
struct Factor {
    float matrix[N][N];
    int pivotRows[N];
    float matrixScale;
};

// Computes P * A = L * U using Doolittle LU decomposition with partial
// pivoting. Input is normalized to keep the factorization scale independent.
template <int N>
static bool FactorizeArray(const float input[N][N], Factor<N> &factor, float tolerance) {
    if (tolerance <= 0.0f || IEEE_FLT_IS_NAN(tolerance)) {
        return false;
    }

    float matrixScale = 0.0f;
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            float value = input[row][column];
            if (IEEE_FLT_IS_NAN(value)) {
                return false;
            }

            float absValue = Math::Fabs(value);
            if (absValue > matrixScale) {
                matrixScale = absValue;
            }
        }
    }

    if (matrixScale == 0.0f) {
        return false;
    }

    Factor<N> result;
    result.matrixScale = matrixScale;
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            result.matrix[row][column] = input[row][column] / matrixScale;
        }
    }

    for (int column = 0; column < N; column++) {
        int pivotRow = column;
        float pivotAbs = Math::Fabs(result.matrix[column][column]);

        for (int row = column + 1; row < N; row++) {
            float candidateAbs = Math::Fabs(result.matrix[row][column]);
            if (candidateAbs > pivotAbs) {
                pivotAbs = candidateAbs;
                pivotRow = row;
            }
        }

        if (pivotAbs <= tolerance || IEEE_FLT_IS_NAN(pivotAbs)) {
            return false;
        }

        result.pivotRows[column] = pivotRow;
        if (pivotRow != column) {
            for (int i = 0; i < N; i++) {
                float value = result.matrix[column][i];
                result.matrix[column][i] = result.matrix[pivotRow][i];
                result.matrix[pivotRow][i] = value;
            }
        }

        float inversePivot = 1.0f / result.matrix[column][column];
        if (IEEE_FLT_IS_NAN(inversePivot)) {
            return false;
        }

        for (int row = column + 1; row < N; row++) {
            result.matrix[row][column] *= inversePivot;
            float multiplier = result.matrix[row][column];
            if (IEEE_FLT_IS_NAN(multiplier)) {
                return false;
            }

            for (int i = column + 1; i < N; i++) {
                result.matrix[row][i] -= multiplier * result.matrix[column][i];
                if (IEEE_FLT_IS_NAN(result.matrix[row][i])) {
                    return false;
                }
            }
        }
    }

    factor = result;
    return true;
}

template <int N, typename MatrixType>
static bool Factorize(const MatrixType &matrix, Factor<N> &factor, float tolerance) {
    float input[N][N];
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            input[row][column] = GetMatrixElement(matrix, row, column);
        }
    }
    return FactorizeArray<N>(input, factor, tolerance);
}

template <int N, typename VectorType>
static bool SolveFactorized(const Factor<N> &factor, const VectorType &rightHandSide, VectorType &solution) {
    if (!rightHandSide.IsFinite()) {
        return false;
    }

    float x[N];
    for (int i = 0; i < N; i++) {
        x[i] = rightHandSide[i] / factor.matrixScale;
        if (IEEE_FLT_IS_NAN(x[i])) {
            return false;
        }
    }

    // Apply the factorization's row permutations to the right-hand side.
    for (int i = 0; i < N; i++) {
        int pivotRow = factor.pivotRows[i];
        if (pivotRow != i) {
            float value = x[i];
            x[i] = x[pivotRow];
            x[pivotRow] = value;
        }
    }

    // Forward substitution for the unit-diagonal lower triangle.
    for (int i = 1; i < N; i++) {
        for (int j = 0; j < i; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
        if (IEEE_FLT_IS_NAN(x[i])) {
            return false;
        }
    }

    // Back substitution for the upper triangle.
    for (int i = N - 1; i >= 0; i--) {
        for (int j = i + 1; j < N; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
        x[i] /= factor.matrix[i][i];
        if (IEEE_FLT_IS_NAN(x[i])) {
            return false;
        }
    }

    VectorType result;
    for (int i = 0; i < N; i++) {
        result[i] = x[i];
    }
    if (!result.IsFinite()) {
        return false;
    }

    solution = result;
    return true;
}

template <int N, typename MatrixType, typename VectorType>
static bool Solve(const MatrixType &matrix, const VectorType &rightHandSide, VectorType &solution, float tolerance) {
    Factor<N> factor;
    return Factorize<N>(matrix, factor, tolerance) && SolveFactorized(factor, rightHandSide, solution);
}

} // namespace LU

BE_NAMESPACE_END
