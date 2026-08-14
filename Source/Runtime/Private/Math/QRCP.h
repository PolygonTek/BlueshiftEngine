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

namespace QRCP {

// Recomputes the norm of a column below the active diagonal. Scaling by the largest
// component prevents very small trailing columns from underflowing while squaring.
template <int N>
static BE_FORCE_INLINE float TrailingColumnNorm(const float matrix[N][N], int firstRow, int column) {
    float scale = 0.0f;
    for (int row = firstRow; row < N; row++) {
        float absValue = Math::Fabs(matrix[row][column]);
        if (absValue > scale) {
            scale = absValue;
        }
    }

    if (scale == 0.0f) {
        return 0.0f;
    }

    float scaledNormSqr = 0.0f;
    for (int row = firstRow; row < N; row++) {
        float scaledValue = matrix[row][column] / scale;
        scaledNormSqr += scaledValue * scaledValue;
    }
    return scale * Math::Sqrt(scaledNormSqr);
}

// Computes the column-pivoted Householder QR factorization input * P = Q * R.
// outputPermutation[j] identifies the original input column at pivoted column j.
template <int N>
static bool Decompose(const float input[N][N], float outputQ[N][N], float outputR[N][N], int outputPermutation[N], int &outputRank, float tolerance) {
    if (tolerance <= 0.0f || !Math::IsFinite(tolerance)) {
        return false;
    }

    float matrixScale = 0.0f;
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            float value = input[row][column];
            if (!Math::IsFinite(value)) {
                return false;
            }

            float absValue = Math::Fabs(value);
            if (absValue > matrixScale) {
                matrixScale = absValue;
            }
        }
    }

    // Global scaling bounds all norm and reflector computations without changing
    // Q, the column permutation, or the numerical rank.
    float q[N][N] = {};
    float r[N][N];
    int permutation[N];
    for (int row = 0; row < N; row++) {
        q[row][row] = 1.0f;
        permutation[row] = row;
        for (int column = 0; column < N; column++) {
            r[row][column] = matrixScale > 0.0f ? input[row][column] / matrixScale : 0.0f;
        }
    }

    if (matrixScale > 0.0f) {
        for (int k = 0; k < N - 1; k++) {
            int pivotColumn = k;
            float maxNorm = TrailingColumnNorm<N>(r, k, k);

            for (int column = k + 1; column < N; column++) {
                float norm = TrailingColumnNorm<N>(r, k, column);
                if (norm > maxNorm) {
                    maxNorm = norm;
                    pivotColumn = column;
                }
            }

            if (!Math::IsFinite(maxNorm)) {
                return false;
            }

            // A zero maximum means every remaining trailing column is already zero.
            if (maxNorm == 0.0f) {
                break;
            }

            if (pivotColumn != k) {
                for (int row = 0; row < N; row++) {
                    float value = r[row][k];
                    r[row][k] = r[row][pivotColumn];
                    r[row][pivotColumn] = value;
                }

                int column = permutation[k];
                permutation[k] = permutation[pivotColumn];
                permutation[pivotColumn] = column;
            }

            float alpha = r[k][k] >= 0.0f ? -maxNorm : maxNorm;

            // Choosing alpha with the opposite sign avoids cancellation in v[k].
            float v[N] = {};
            for (int row = k; row < N; row++) {
                v[row] = r[row][k];
            }
            v[k] -= alpha;

            // H is invariant to scaling v. Normalize it by its largest component so
            // v^T * v and its reciprocal stay representable for tiny trailing columns.
            float vScale = 0.0f;
            for (int row = k; row < N; row++) {
                float absValue = Math::Fabs(v[row]);
                if (absValue > vScale) {
                    vScale = absValue;
                }
            }

            if (vScale == 0.0f || !Math::IsFinite(vScale)) {
                return false;
            }

            float vNormSqr = 0.0f;
            for (int row = k; row < N; row++) {
                v[row] /= vScale;
                vNormSqr += v[row] * v[row];
            }

            float beta = 2.0f / vNormSqr;
            if (!Math::IsFinite(beta)) {
                return false;
            }

            // Pre-multiply the active block by H = I - beta * v * v^T.
            for (int column = k; column < N; column++) {
                float dot = 0.0f;
                for (int row = k; row < N; row++) {
                    dot += v[row] * r[row][column];
                }
                float factor = beta * dot;
                for (int row = k; row < N; row++) {
                    r[row][column] -= factor * v[row];
                }
            }
            r[k][k] = alpha;
            for (int row = k + 1; row < N; row++) {
                r[row][k] = 0.0f;
            }

            // Q accumulates the same symmetric reflector on the right.
            for (int row = 0; row < N; row++) {
                float dot = 0.0f;
                for (int column = k; column < N; column++) {
                    dot += q[row][column] * v[column];
                }
                float factor = beta * dot;
                for (int column = k; column < N; column++) {
                    q[row][column] -= factor * v[column];
                }
            }
        }
    }

    // QR signs are arbitrary. Choose non-negative diagonal entries in R for a
    // deterministic representative while preserving Q * R.
    for (int i = 0; i < N; i++) {
        if (r[i][i] < 0.0f) {
            for (int row = 0; row < N; row++) {
                q[row][i] = -q[row][i];
            }
            for (int column = i; column < N; column++) {
                r[i][column] = -r[i][column];
            }
        }
    }

    float maxDiagonal = 0.0f;
    for (int i = 0; i < N; i++) {
        float diagonal = Math::Fabs(r[i][i]);
        if (diagonal > maxDiagonal) {
            maxDiagonal = diagonal;
        }
    }

    int rank = 0;
    float rankThreshold = tolerance * maxDiagonal;
    for (int i = 0; i < N; i++) {
        if (Math::Fabs(r[i][i]) > rankThreshold) {
            rank++;
        }
    }

    // Commit only after validating the fully restored factors.
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            float qValue = q[row][column];
            float rValue = r[row][column] * matrixScale;
            if (!Math::IsFinite(qValue) || !Math::IsFinite(rValue)) {
                return false;
            }
            outputQ[row][column] = qValue;
            outputR[row][column] = rValue;
        }
        outputPermutation[row] = permutation[row];
    }
    outputRank = rank;
    return true;
}

} // namespace QRCP

BE_NAMESPACE_END
