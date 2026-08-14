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

namespace SymmetricPSDSolver {

static BE_FORCE_INLINE float GetMatrixElement(const Mat2 &matrix, int row, int column) {
    return matrix[row][column];
}

static BE_FORCE_INLINE float GetMatrixElement(const Mat3 &matrix, int row, int column) {
    return matrix[column][row];
}

static BE_FORCE_INLINE float GetMatrixElement(const Mat4 &matrix, int row, int column) {
    return matrix[row][column];
}

template <int N, typename MatrixType, typename VectorType>
static bool SolveCholeskyFactor(const MatrixType &lowerTriangular, const VectorType &rightHandSide, VectorType &solution) {
    float values[N];

    // Forward substitution solves L * y = b.
    for (int row = 0; row < N; row++) {
        float value = rightHandSide[row];
        for (int column = 0; column < row; column++) {
            value -= GetMatrixElement(lowerTriangular, row, column) * values[column];
        }

        value /= GetMatrixElement(lowerTriangular, row, row);
        if (!Math::IsFinite(value)) {
            return false;
        }
        values[row] = value;
    }

    // Back substitution solves L^T * x = y in the same stack buffer.
    for (int row = N - 1; row >= 0; row--) {
        float value = values[row];
        for (int column = row + 1; column < N; column++) {
            value -= GetMatrixElement(lowerTriangular, column, row) * values[column];
        }

        value /= GetMatrixElement(lowerTriangular, row, row);
        if (!Math::IsFinite(value)) {
            return false;
        }
        values[row] = value;
    }

    VectorType result;
    for (int i = 0; i < N; i++) {
        result[i] = values[i];
    }
    if (!result.IsFinite()) {
        return false;
    }

    solution = result;
    return true;
}

template <int N, typename MatrixType, typename VectorType>
static bool Solve(const MatrixType &matrix, const VectorType &rightHandSide, VectorType &solution, float tolerance) {
    if (tolerance <= 0.0f || !Math::IsFinite(tolerance) || !rightHandSide.IsFinite()) {
        return false;
    }

    // Positive-definite matrices take the cheaper Cholesky path and require only
    // one forward and one back substitution after factorization.
    MatrixType lowerTriangular;
    if (matrix.CholeskyDecompose(lowerTriangular, tolerance)) {
        return SolveCholeskyFactor<N>(lowerTriangular, rightHandSide, solution);
    }

    // Cholesky failure is not an immediate error: a valid positive semi-definite
    // matrix may be singular (or numerically rank-deficient at this tolerance).
    // The symmetric eigendecomposition fallback separates that case from invalid,
    // non-symmetric, or indefinite input and provides its numerical range space.
    VectorType eigenValues;
    MatrixType eigenVectors;
    if (!matrix.EigenDecomposeSymmetric(eigenValues, eigenVectors, tolerance)) {
        return false;
    }

    float eigenvalueScale = 0.0f;
    for (int i = 0; i < N; i++) {
        eigenvalueScale = Max(eigenvalueScale, Math::Fabs(eigenValues[i]));
    }
    // Reject materially negative eigenvalues. Values within the relative zero
    // threshold are treated as null-space directions for the minimum-norm solution.
    float zeroThreshold = tolerance * eigenvalueScale;
    if (eigenValues[N - 1] < -zeroThreshold) {
        return false;
    }

    VectorType coefficients = eigenVectors.Transpose() * rightHandSide;
    for (int i = 0; i < N; i++) {
        coefficients[i] = eigenValues[i] > zeroThreshold ? coefficients[i] / eigenValues[i] : 0.0f;
    }

    VectorType result = eigenVectors * coefficients;
    if (!result.IsFinite()) {
        return false;
    }

    solution = result;
    return true;
}

} // namespace SymmetricPSDSolver

BE_NAMESPACE_END
