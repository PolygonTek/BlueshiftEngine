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

// Recomputes the squared norm of a column below the active diagonal. Fixed-size
// matrices are small enough that this is preferable to an approximate norm downdate.
template <int N>
static BE_FORCE_INLINE float TrailingColumnNormSqr(const float matrix[N][N], int firstRow, int column) {
    float normSqr = 0.0f;
    for (int row = firstRow; row < N; row++) {
        normSqr += matrix[row][column] * matrix[row][column];
    }
    return normSqr;
}

// Computes the column-pivoted Householder QR factorization input * P = Q * R.
// outputPermutation[j] identifies the original input column at pivoted column j.
template <int N>
static bool Decompose(const float input[N][N], float outputQ[N][N], float outputR[N][N], int outputPermutation[N], int &outputRank, float tolerance) {
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
            float maxNormSqr = TrailingColumnNormSqr<N>(r, k, k);

            for (int column = k + 1; column < N; column++) {
                float normSqr = TrailingColumnNormSqr<N>(r, k, column);
                if (normSqr > maxNormSqr) {
                    maxNormSqr = normSqr;
                    pivotColumn = column;
                }
            }

            if (IEEE_FLT_IS_NAN(maxNormSqr)) {
                return false;
            }

            // A zero maximum means every remaining trailing column is already zero.
            if (maxNormSqr == 0.0f) {
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

            float norm = Math::Sqrt(maxNormSqr);
            float alpha = r[k][k] >= 0.0f ? -norm : norm;

            // Choosing alpha with the opposite sign avoids cancellation in v[k].
            float v[N] = {};
            float vNormSqr = 0.0f;
            for (int row = k; row < N; row++) {
                v[row] = r[row][k];
            }
            v[k] -= alpha;
            for (int row = k; row < N; row++) {
                vNormSqr += v[row] * v[row];
            }

            if (vNormSqr <= 0.0f || IEEE_FLT_IS_NAN(vNormSqr)) {
                return false;
            }

            float beta = 2.0f / vNormSqr;
            if (IEEE_FLT_IS_NAN(beta)) {
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
            if (IEEE_FLT_IS_NAN(qValue) || IEEE_FLT_IS_NAN(rValue)) {
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
