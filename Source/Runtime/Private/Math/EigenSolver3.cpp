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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

struct LUFactor3 {
    float matrix[3][3];
    int pivotRows[3];
};

static bool ComputeEigenPair(const Mat3 &matrix, const Vec3 &eigenVector, int numIterations, EigenPair3 &result) {
    Vec3 transformed = matrix * eigenVector;
    float eigenValue = eigenVector.Dot(transformed);
    float residual = (transformed - eigenVector * eigenValue).SafeLength();

    if (IEEE_FLT_IS_NAN(eigenValue) || IEEE_FLT_IS_NAN(residual)) {
        return false;
    }

    result.eigenValue = eigenValue;
    result.eigenVector = eigenVector;
    result.residual = residual;
    result.numIterations = numIterations;
    return true;
}

static bool ValidateIterationInput(const Mat3 &matrix, const Vec3 &initialVector, int maxIterations, float tolerance) {
    return maxIterations > 0 && tolerance > 0.0f && !IEEE_FLT_IS_NAN(tolerance) && matrix.IsFinite() && initialVector.IsFinite();
}

static bool FactorShiftedMatrix(const Mat3 &matrix, float shift, LUFactor3 &factor) {
    float matrixScale = 0.0f;
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 3; column++) {
            float value = matrix[column][row] - (row == column ? shift : 0.0f);
            if (IEEE_FLT_IS_NAN(value)) {
                return false;
            }

            factor.matrix[row][column] = value;
            float absValue = Math::Fabs(value);
            if (absValue > matrixScale) {
                matrixScale = absValue;
            }
        }
    }

    if (matrixScale == 0.0f) {
        return false;
    }

    float pivotTolerance = FLT_EPSILON * matrixScale;

    // LU decomposition with partial pivoting of A - shift * I.
    for (int column = 0; column < 3; column++) {
        int pivotRow = column;
        float pivotAbs = Math::Fabs(factor.matrix[column][column]);

        for (int row = column + 1; row < 3; row++) {
            float candidateAbs = Math::Fabs(factor.matrix[row][column]);
            if (candidateAbs > pivotAbs) {
                pivotAbs = candidateAbs;
                pivotRow = row;
            }
        }

        if (pivotAbs <= pivotTolerance) {
            return false;
        }

        factor.pivotRows[column] = pivotRow;
        if (pivotRow != column) {
            for (int i = 0; i < 3; i++) {
                float value = factor.matrix[column][i];
                factor.matrix[column][i] = factor.matrix[pivotRow][i];
                factor.matrix[pivotRow][i] = value;
            }
        }

        float inversePivot = 1.0f / factor.matrix[column][column];
        for (int row = column + 1; row < 3; row++) {
            factor.matrix[row][column] *= inversePivot;
            float multiplier = factor.matrix[row][column];
            for (int i = column + 1; i < 3; i++) {
                factor.matrix[row][i] -= multiplier * factor.matrix[column][i];
            }
        }
    }
    return true;
}

static bool SolveFactorizedMatrix(const LUFactor3 &factor, const Vec3 &rightHandSide, Vec3 &solution) {
    float x[3] = { rightHandSide.x, rightHandSide.y, rightHandSide.z };

    // Apply the factorization's row permutations to the right-hand side.
    for (int i = 0; i < 3; i++) {
        int pivotRow = factor.pivotRows[i];
        if (pivotRow != i) {
            float value = x[i];
            x[i] = x[pivotRow];
            x[pivotRow] = value;
        }
    }

    // Forward substitution for the unit-diagonal lower triangle.
    for (int i = 1; i < 3; i++) {
        for (int j = 0; j < i; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
    }

    // Back substitution for the upper triangle.
    for (int i = 2; i >= 0; i--) {
        for (int j = i + 1; j < 3; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
        x[i] /= factor.matrix[i][i];
    }

    solution.Set(x[0], x[1], x[2]);
    return solution.IsFinite();
}

bool EigenSolver3::PowerIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance)) {
        return false;
    }

    Vec3 vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    float matrixScale = matrix.MaxAbsComponent();
    if (matrixScale == 0.0f) {
        EigenPair3 zeroResult;
        zeroResult.eigenValue = 0.0f;
        zeroResult.eigenVector = vector;
        zeroResult.residual = 0.0f;
        zeroResult.numIterations = 0;
        result = zeroResult;
        return true;
    }

    float residualTolerance = tolerance * matrixScale;
    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        Vec3 nextVector = matrix * vector;
        if (!nextVector.SafeNormalize()) {
            return false;
        }

        EigenPair3 candidate;
        if (!ComputeEigenPair(matrix, nextVector, iteration, candidate)) {
            return false;
        }

        if (candidate.residual <= residualTolerance) {
            result = candidate;
            return true;
        }
        vector = nextVector;
    }
    return false;
}

bool EigenSolver3::InversePowerIteration(const Mat3 &matrix, float shift, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance) ||
        IEEE_FLT_IS_NAN(shift)) {
        return false;
    }

    LUFactor3 factor;
    if (!FactorShiftedMatrix(matrix, shift, factor)) {
        return false;
    }

    Vec3 vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    float residualTolerance = tolerance * matrix.MaxAbsComponent();
    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        Vec3 nextVector;
        if (!SolveFactorizedMatrix(factor, vector, nextVector) || !nextVector.SafeNormalize()) {
            return false;
        }

        EigenPair3 candidate;
        if (!ComputeEigenPair(matrix, nextVector, iteration, candidate)) {
            return false;
        }

        if (candidate.residual <= residualTolerance) {
            result = candidate;
            return true;
        }
        vector = nextVector;
    }
    return false;
}

bool EigenSolver3::RayleighQuotientIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance)) {
        return false;
    }

    float matrixScale = matrix.MaxAbsComponent();
    if (matrixScale > 0.0f && !matrix.IsSymmetric(tolerance * matrixScale)) {
        return false;
    }

    Vec3 vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    EigenPair3 candidate;
    if (!ComputeEigenPair(matrix, vector, 0, candidate)) {
        return false;
    }

    float residualTolerance = tolerance * matrixScale;
    if (candidate.residual <= residualTolerance) {
        result = candidate;
        return true;
    }

    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        LUFactor3 factor;
        if (!FactorShiftedMatrix(matrix, candidate.eigenValue, factor)) {
            return false;
        }

        Vec3 nextVector;
        if (!SolveFactorizedMatrix(factor, vector, nextVector) || !nextVector.SafeNormalize()) {
            return false;
        }

        if (!ComputeEigenPair(matrix, nextVector, iteration, candidate)) {
            return false;
        }

        if (candidate.residual <= residualTolerance) {
            result = candidate;
            return true;
        }
        vector = nextVector;
    }
    return false;
}

BE_NAMESPACE_END
