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

namespace EigenSolverFixed {

static BE_FORCE_INLINE float GetMatrixElement(const Mat3 &matrix, int row, int column) {
    return matrix[column][row];
}

static BE_FORCE_INLINE float GetMatrixElement(const Mat4 &matrix, int row, int column) {
    return matrix[row][column];
}

template <int Dimension>
struct LUFactor {
    float matrix[Dimension][Dimension];
    int pivotRows[Dimension];
};

template <typename MatrixType, typename VectorType, typename EigenPairType>
static bool ComputeEigenPair(const MatrixType &matrix, const VectorType &eigenVector, int numIterations, EigenPairType &result) {
    VectorType transformed = matrix * eigenVector;
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

template <typename MatrixType, typename VectorType>
static bool ValidateIterationInput(const MatrixType &matrix, const VectorType &initialVector, int maxIterations, float tolerance) {
    return maxIterations > 0 && tolerance > 0.0f && !IEEE_FLT_IS_NAN(tolerance) && matrix.IsFinite() && initialVector.IsFinite();
}

template <int Dimension, typename MatrixType>
static bool FactorShiftedMatrix(const MatrixType &matrix, float shift, LUFactor<Dimension> &factor) {
    float matrixScale = 0.0f;
    for (int row = 0; row < Dimension; row++) {
        for (int column = 0; column < Dimension; column++) {
            float value = GetMatrixElement(matrix, row, column) - (row == column ? shift : 0.0f);
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
    for (int column = 0; column < Dimension; column++) {
        int pivotRow = column;
        float pivotAbs = Math::Fabs(factor.matrix[column][column]);

        for (int row = column + 1; row < Dimension; row++) {
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
            for (int i = 0; i < Dimension; i++) {
                float value = factor.matrix[column][i];
                factor.matrix[column][i] = factor.matrix[pivotRow][i];
                factor.matrix[pivotRow][i] = value;
            }
        }

        float inversePivot = 1.0f / factor.matrix[column][column];
        for (int row = column + 1; row < Dimension; row++) {
            factor.matrix[row][column] *= inversePivot;
            float multiplier = factor.matrix[row][column];
            for (int i = column + 1; i < Dimension; i++) {
                factor.matrix[row][i] -= multiplier * factor.matrix[column][i];
            }
        }
    }
    return true;
}

template <int Dimension, typename VectorType>
static bool SolveFactorizedMatrix(const LUFactor<Dimension> &factor, const VectorType &rightHandSide, VectorType &solution) {
    float x[Dimension];
    for (int i = 0; i < Dimension; i++) {
        x[i] = rightHandSide[i];
    }

    // Apply the factorization's row permutations to the right-hand side.
    for (int i = 0; i < Dimension; i++) {
        int pivotRow = factor.pivotRows[i];
        if (pivotRow != i) {
            float value = x[i];
            x[i] = x[pivotRow];
            x[pivotRow] = value;
        }
    }

    // Forward substitution for the unit-diagonal lower triangle.
    for (int i = 1; i < Dimension; i++) {
        for (int j = 0; j < i; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
    }

    // Back substitution for the upper triangle.
    for (int i = Dimension - 1; i >= 0; i--) {
        for (int j = i + 1; j < Dimension; j++) {
            x[i] -= factor.matrix[i][j] * x[j];
        }
        x[i] /= factor.matrix[i][i];
    }

    for (int i = 0; i < Dimension; i++) {
        solution[i] = x[i];
    }
    return solution.IsFinite();
}

template <int Dimension, typename MatrixType, typename VectorType, typename EigenPairType>
static bool PowerIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPairType &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance)) {
        return false;
    }

    VectorType vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    float matrixScale = matrix.MaxAbsComponent();
    if (matrixScale == 0.0f) {
        EigenPairType zeroResult;
        zeroResult.eigenValue = 0.0f;
        zeroResult.eigenVector = vector;
        zeroResult.residual = 0.0f;
        zeroResult.numIterations = 0;
        result = zeroResult;
        return true;
    }

    float residualTolerance = tolerance * matrixScale;
    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        VectorType nextVector = matrix * vector;
        if (!nextVector.SafeNormalize()) {
            return false;
        }

        EigenPairType candidate;
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

template <int Dimension, typename MatrixType, typename VectorType, typename EigenPairType>
static bool InversePowerIteration(const MatrixType &matrix, float shift, const VectorType &initialVector, EigenPairType &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance) ||
        IEEE_FLT_IS_NAN(shift)) {
        return false;
    }

    LUFactor<Dimension> factor;
    if (!FactorShiftedMatrix(matrix, shift, factor)) {
        return false;
    }

    VectorType vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    float residualTolerance = tolerance * matrix.MaxAbsComponent();
    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        VectorType nextVector;
        if (!SolveFactorizedMatrix(factor, vector, nextVector) || !nextVector.SafeNormalize()) {
            return false;
        }

        EigenPairType candidate;
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

template <int Dimension, typename MatrixType, typename VectorType, typename EigenPairType>
static bool RayleighQuotientIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPairType &result, int maxIterations, float tolerance) {
    if (!ValidateIterationInput(matrix, initialVector, maxIterations, tolerance)) {
        return false;
    }

    float matrixScale = matrix.MaxAbsComponent();
    if (matrixScale > 0.0f && !matrix.IsSymmetric(tolerance * matrixScale)) {
        return false;
    }

    VectorType vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    EigenPairType candidate;
    if (!ComputeEigenPair(matrix, vector, 0, candidate)) {
        return false;
    }

    float residualTolerance = tolerance * matrixScale;
    if (candidate.residual <= residualTolerance) {
        result = candidate;
        return true;
    }

    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        LUFactor<Dimension> factor;
        if (!FactorShiftedMatrix(matrix, candidate.eigenValue, factor)) {
            return false;
        }

        VectorType nextVector;
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

}

BE_NAMESPACE_END
