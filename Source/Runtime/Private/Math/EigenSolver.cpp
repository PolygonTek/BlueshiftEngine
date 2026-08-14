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
#include "EigenSolverFixed.h"

BE_NAMESPACE_BEGIN

template <typename VectorType, typename MatrixType>
bool EigenSolver<VectorType, MatrixType>::PowerIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::PowerIteration<MatrixType::Rows>(matrix, initialVector, result, maxIterations, tolerance);
}

template <typename VectorType, typename MatrixType>
bool EigenSolver<VectorType, MatrixType>::InversePowerIteration(const MatrixType &matrix, float shift, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::InversePowerIteration<MatrixType::Rows>(matrix, shift, initialVector, result, maxIterations, tolerance);
}

template <typename VectorType, typename MatrixType>
bool EigenSolver<VectorType, MatrixType>::RayleighQuotientIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::RayleighQuotientIteration<MatrixType::Rows>(matrix, initialVector, result, maxIterations, tolerance);
}

template struct EigenSolver<Vec3, Mat3>;
template struct EigenSolver<Vec4, Mat4>;

BE_NAMESPACE_END
