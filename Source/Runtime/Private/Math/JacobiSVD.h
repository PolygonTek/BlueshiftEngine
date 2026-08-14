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

#include "JacobiRotation.h"

BE_NAMESPACE_BEGIN

namespace JacobiSVD {

// Returns an entry of the column Gram matrix A^T A without forming it explicitly.
template <int N>
static float ColumnDot(const float matrix[N][N], int column0, int column1) {
    float dot = 0.0f;
    for (int row = 0; row < N; row++) {
        dot += matrix[row][column0] * matrix[row][column1];
    }
    return dot;
}

// Tests whether the nonzero columns are mutually orthogonal to the requested tolerance.
// The input has already been scaled by its largest element, so columns whose squared
// norms are below tolerance^2 will later be treated as numerical null-space columns.
template <int N>
static bool ColumnsAreOrthogonal(const float matrix[N][N], float tolerance) {
    float zeroColumnNormSqr = tolerance * tolerance;
    for (int p = 0; p < N - 1; p++) {
        float alpha = ColumnDot<N>(matrix, p, p);
        for (int q = p + 1; q < N; q++) {
            if (alpha <= zeroColumnNormSqr) {
                continue;
            }

            float beta = ColumnDot<N>(matrix, q, q);
            float gamma = ColumnDot<N>(matrix, p, q);
            if (beta <= zeroColumnNormSqr) {
                continue;
            }

            float threshold = tolerance * Math::Sqrt(alpha * beta);
            if (Math::Fabs(gamma) > threshold) {
                return false;
            }
        }
    }
    return true;
}

// Deterministically completes one column of an orthonormal basis. This is used for
// left singular vectors associated with zero singular values, which are not defined
// by A * v / sigma. Two MGS passes reduce loss of orthogonality.
template <int N>
static bool CompleteOrthonormalColumn(float matrix[N][N], int column) {
    float bestVector[N];
    float bestLengthSqr = 0.0f;

    for (int basisIndex = 0; basisIndex < N; basisIndex++) {
        float candidate[N] = {};
        candidate[basisIndex] = 1.0f;

        // Two modified Gram-Schmidt passes reduce loss of orthogonality.
        for (int pass = 0; pass < 2; pass++) {
            for (int previousColumn = 0; previousColumn < column; previousColumn++) {
                float projection = 0.0f;
                for (int row = 0; row < N; row++) {
                    projection += candidate[row] * matrix[row][previousColumn];
                }
                for (int row = 0; row < N; row++) {
                    candidate[row] -= projection * matrix[row][previousColumn];
                }
            }
        }

        float lengthSqr = 0.0f;
        for (int row = 0; row < N; row++) {
            lengthSqr += candidate[row] * candidate[row];
        }
        if (lengthSqr > bestLengthSqr) {
            bestLengthSqr = lengthSqr;
            for (int row = 0; row < N; row++) {
                bestVector[row] = candidate[row];
            }
        }
    }

    if (bestLengthSqr <= FLT_EPSILON || IEEE_FLT_IS_NAN(bestLengthSqr)) {
        return false;
    }

    float inverseLength = Math::InvSqrt(bestLengthSqr);
    for (int row = 0; row < N; row++) {
        matrix[row][column] = bestVector[row] * inverseLength;
    }
    return true;
}

// Computes the fixed-size one-sided Jacobi SVD
//
//     input = outputU * diag(outputSingularValues) * outputV^T
//
// Repeatedly orthogonalizes pairs of input columns without forming input^T * input.
// Mat3 and Mat4 share this implementation, which uses only fixed-size stack storage.
template <int N>
static bool Decompose(const float input[N][N], float outputU[N][N], float outputSingularValues[N], float outputV[N][N], float tolerance) {
    if (tolerance <= 0.0f || IEEE_FLT_IS_NAN(tolerance)) {
        return false;
    }

    // Global scaling keeps Gram matrix entries finite without changing singular vectors.
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

    // working = input / matrixScale * V. V starts as identity and accumulates every
    // right-side Jacobi rotation.
    float working[N][N];
    float rightVectors[N][N] = {};
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            working[row][column] = matrixScale > 0.0f ? input[row][column] / matrixScale : 0.0f;
        }
        rightVectors[row][row] = 1.0f;
    }

    if (matrixScale > 0.0f) {
        // Cyclic sweeps converge quickly for 2x2, 3x3, and 4x4 matrices; the fixed upper
        // bound prevents malformed inputs from causing an unbounded iteration.
        const int maxSweeps = 32;
        bool converged = false;

        for (int sweep = 0; sweep < maxSweeps; sweep++) {
            bool rotated = false;

            for (int p = 0; p < N - 1; p++) {
                for (int q = p + 1; q < N; q++) {
                    // alpha, beta, and gamma are the 2x2 Gram matrix entries for
                    // the selected column pair.
                    float alpha = ColumnDot<N>(working, p, p);
                    float beta = ColumnDot<N>(working, q, q);
                    float gamma = ColumnDot<N>(working, p, q);
                    float threshold = tolerance * Math::Sqrt(alpha * beta);
                    float zeroColumnNormSqr = tolerance * tolerance;

                    // A nearly null column already belongs to the numerical null space.
                    // Continuing relative rotations against it only amplifies roundoff.
                    if (alpha <= zeroColumnNormSqr || beta <= zeroColumnNormSqr) {
                        continue;
                    }

                    if (Math::Fabs(gamma) <= threshold || gamma == 0.0f) {
                        continue;
                    }

                    JacobiRotation::Coefficients rotation = JacobiRotation::ComputeSymmetric(alpha, beta, gamma);
                    JacobiRotation::RotateColumns<N>(working, p, q, rotation);
                    JacobiRotation::RotateColumns<N>(rightVectors, p, q, rotation);
                    rotated = true;
                }
            }

            // A sweep is complete when no pair needs a rotation, or when a final
            // Gram-matrix check confirms that all significant columns are orthogonal.
            if (!rotated || ColumnsAreOrthogonal<N>(working, tolerance)) {
                converged = true;
                break;
            }
        }

        if (!converged && !ColumnsAreOrthogonal<N>(working, tolerance)) {
            return false;
        }
    }

    // Once the columns are orthogonal, their lengths are the singular values of
    // the globally scaled input matrix.
    float scaledSingularValues[N];
    for (int column = 0; column < N; column++) {
        float lengthSqr = ColumnDot<N>(working, column, column);
        if (lengthSqr < 0.0f || IEEE_FLT_IS_NAN(lengthSqr)) {
            return false;
        }
        scaledSingularValues[column] = Math::Sqrt(lengthSqr);
    }

    // Sort singular values in descending order and keep both vector sets paired.
    for (int i = 0; i < N - 1; i++) {
        int maxIndex = i;
        for (int j = i + 1; j < N; j++) {
            if (scaledSingularValues[j] > scaledSingularValues[maxIndex]) {
                maxIndex = j;
            }
        }

        if (maxIndex != i) {
            float singularValue = scaledSingularValues[i];
            scaledSingularValues[i] = scaledSingularValues[maxIndex];
            scaledSingularValues[maxIndex] = singularValue;

            for (int row = 0; row < N; row++) {
                float value = working[row][i];
                working[row][i] = working[row][maxIndex];
                working[row][maxIndex] = value;

                value = rightVectors[row][i];
                rightVectors[row][i] = rightVectors[row][maxIndex];
                rightVectors[row][maxIndex] = value;
            }
        }
    }

    // Singular values at or below tolerance * sigma_max are treated as zero.
    float leftVectors[N][N] = {};
    float zeroThreshold = tolerance * scaledSingularValues[0];
    int nonzeroCount = 0;

    for (int column = 0; column < N; column++) {
        float singularValue = scaledSingularValues[column];
        if (singularValue <= zeroThreshold) {
            scaledSingularValues[column] = 0.0f;
            continue;
        }

        float inverseSingularValue = 1.0f / singularValue;
        int maxAbsIndex = 0;
        float maxAbsValue = 0.0f;
        for (int row = 0; row < N; row++) {
            leftVectors[row][column] = working[row][column] * inverseSingularValue;
            float absValue = Math::Fabs(leftVectors[row][column]);
            if (absValue > maxAbsValue) {
                maxAbsValue = absValue;
                maxAbsIndex = row;
            }
        }

        // SVD vector signs are arbitrary; choose a deterministic representative.
        if (leftVectors[maxAbsIndex][column] < 0.0f) {
            for (int row = 0; row < N; row++) {
                leftVectors[row][column] *= -1.0f;
                rightVectors[row][column] *= -1.0f;
            }
        }
        nonzeroCount++;
    }

    // Null-space left singular vectors are arbitrary; complete them with a stable,
    // deterministic orthonormal basis.
    for (int column = nonzeroCount; column < N; column++) {
        if (!CompleteOrthonormalColumn<N>(leftVectors, column)) {
            return false;
        }
    }

    // Restore the original magnitude only after all vector computations are complete.
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            outputU[row][column] = leftVectors[row][column];
            outputV[row][column] = rightVectors[row][column];
        }
        outputSingularValues[row] = scaledSingularValues[row] * matrixScale;
        if (IEEE_FLT_IS_NAN(outputSingularValues[row])) {
            return false;
        }
    }
    return true;
}

template <int N, typename MatrixType, typename VectorType>
static bool ComputeConditionNumber(const MatrixType &matrix, float &conditionNumber, float tolerance) {
    MatrixType leftVectors;
    VectorType singularValues;
    MatrixType rightVectors;
    if (!matrix.SingularValueDecompose(leftVectors, singularValues, rightVectors, tolerance)) {
        return false;
    }

    float result = INFINITY;
    if (singularValues[0] > 0.0f && singularValues[N - 1] > 0.0f) {
        result = singularValues[0] / singularValues[N - 1];
    }

    conditionNumber = result;
    return true;
}

template <int N, typename MatrixType, typename VectorType>
static bool ComputePseudoInverse(const MatrixType &matrix, MatrixType &pseudoInverse, float tolerance) {
    MatrixType leftVectors;
    VectorType singularValues;
    MatrixType rightVectors;
    if (!matrix.SingularValueDecompose(leftVectors, singularValues, rightVectors, tolerance)) {
        return false;
    }

    MatrixType inverseSingularValues = MatrixType::zero;
    for (int i = 0; i < N; i++) {
        if (singularValues[i] > 0.0f) {
            inverseSingularValues[i][i] = 1.0f / singularValues[i];
        }
    }

    MatrixType result = rightVectors * inverseSingularValues;
    result *= leftVectors.Transpose();
    if (!result.IsFinite()) {
        return false;
    }

    pseudoInverse = result;
    return true;
}

} // namespace JacobiSVD

BE_NAMESPACE_END
