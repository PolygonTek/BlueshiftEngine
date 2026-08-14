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

#include "LU.h"

BE_NAMESPACE_BEGIN

namespace EigenSolverFixed {

// Uses the Rayleigh quotient as the eigenvalue estimate and ||A v - lambda v||
// as the convergence residual for the normalized candidate vector.
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

// Factors A - shift * I with partial pivoting. Failure means the shifted
// system is non-finite or numerically singular at the selected shift.
template <int Dimension, typename MatrixType>
static bool FactorShiftedMatrix(const MatrixType &matrix, float shift, LU::Factor<Dimension> &factor) {
    float shiftedMatrix[Dimension][Dimension];
    for (int row = 0; row < Dimension; row++) {
        for (int column = 0; column < Dimension; column++) {
            shiftedMatrix[row][column] = LU::GetMatrixElement(matrix, row, column) - (row == column ? shift : 0.0f);
        }
    }
    return LU::FactorizeArray<Dimension>(shiftedMatrix, factor, FLT_EPSILON);
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
    // Repeated multiplication amplifies the eigenvector associated with the
    // eigenvalue of largest magnitude.
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

    LU::Factor<Dimension> factor;
    if (!FactorShiftedMatrix(matrix, shift, factor)) {
        return false;
    }

    // The shift is fixed, so reuse its LU factors. Applying the inverse favors
    // the eigenvector whose eigenvalue is closest to that shift.
    VectorType vector = initialVector;
    if (!vector.SafeNormalize()) {
        return false;
    }

    float residualTolerance = tolerance * matrix.MaxAbsComponent();
    for (int iteration = 1; iteration <= maxIterations; iteration++) {
        VectorType nextVector;
        if (!LU::SolveFactorized(factor, vector, nextVector) || !nextVector.SafeNormalize()) {
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
        // The current Rayleigh quotient becomes the next shift, requiring a
        // new factorization of the shifted system on every iteration.
        LU::Factor<Dimension> factor;
        if (!FactorShiftedMatrix(matrix, candidate.eigenValue, factor)) {
            return false;
        }

        VectorType nextVector;
        if (!LU::SolveFactorized(factor, vector, nextVector) || !nextVector.SafeNormalize()) {
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
