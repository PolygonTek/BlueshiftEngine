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

namespace Cholesky {

// Computes input = outputLower * outputLower^T for a finite symmetric positive-
// definite matrix. The implementation uses fixed-size row-major stack storage.
template <int N>
static bool Decompose(const float input[N][N], float outputLower[N][N], float tolerance) {
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

            float absValue = Math::Abs(value);
            if (absValue > matrixScale) {
                matrixScale = absValue;
            }
        }
    }

    if (matrixScale == 0.0f) {
        return false;
    }

    for (int row = 1; row < N; row++) {
        for (int column = 0; column < row; column++) {
            float difference = input[row][column] - input[column][row];
            if (!Math::IsFinite(difference) || Math::Abs(difference) > tolerance) {
                return false;
            }
        }
    }

    // Normalize the input so intermediate products remain bounded. Averaging the
    // two triangles removes asymmetry already accepted by the tolerance check.
    float working[N][N];
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            working[row][column] = (0.5f * input[row][column] + 0.5f * input[column][row]) / matrixScale;
        }
    }

    float lower[N][N] = {};
    for (int row = 0; row < N; row++) {
        for (int column = 0; column <= row; column++) {
            float sum = working[row][column];
            for (int k = 0; k < column; k++) {
                sum -= lower[row][k] * lower[column][k];
            }

            if (!Math::IsFinite(sum)) {
                return false;
            }

            if (row == column) {
                // A non-positive or numerically negligible pivot means the input
                // is not positive definite at the requested relative tolerance.
                if (sum <= tolerance) {
                    return false;
                }
                lower[row][column] = Math::Sqrt(sum);
            } else {
                lower[row][column] = sum / lower[column][column];
                if (!Math::IsFinite(lower[row][column])) {
                    return false;
                }
            }
        }
    }

    float lowerScale = Math::Sqrt(matrixScale);
    if (!Math::IsFinite(lowerScale)) {
        return false;
    }

    // Commit the result only after the complete factorization succeeds.
    for (int row = 0; row < N; row++) {
        for (int column = 0; column < N; column++) {
            float value = lower[row][column] * lowerScale;
            if (!Math::IsFinite(value)) {
                return false;
            }
            outputLower[row][column] = value;
        }
    }
    return true;
}

} // namespace Cholesky

BE_NAMESPACE_END
