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

#include "Math/Matrix3.h"
#include "Math/Matrix4.h"

BE_NAMESPACE_BEGIN

template <typename VectorType>
struct EigenPair {
    float               eigenValue;
    VectorType          eigenVector;
    float               residual;
    int                 numIterations;
};

template <typename VectorType, typename MatrixType>
struct BE_API EigenSolver {
                        /// Finds the real eigenpair with the largest eigenvalue magnitude using power iteration.
                        /// The result is left unchanged if the iteration fails to converge.
    static bool         PowerIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations = 32, float tolerance = MATRIX_EPSILON);

                        /// Finds the real eigenpair nearest to shift using shifted inverse power iteration.
                        /// The shifted matrix is factored once and solved without explicitly forming its inverse.
                        /// The result is left unchanged if factorization or iteration fails.
    static bool         InversePowerIteration(const MatrixType &matrix, float shift, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations = 32, float tolerance = MATRIX_EPSILON);

                        /// Refines an eigenpair of a real symmetric matrix using Rayleigh quotient iteration.
                        /// The shift is recomputed from the current vector on every iteration.
                        /// The result is left unchanged if validation, factorization, or iteration fails.
    static bool         RayleighQuotientIteration(const MatrixType &matrix, const VectorType &initialVector, EigenPair<VectorType> &result, int maxIterations = 16, float tolerance = MATRIX_EPSILON);
};

using EigenPair3 = EigenPair<Vec3>;
using EigenPair4 = EigenPair<Vec4>;
using EigenSolver3 = EigenSolver<Vec3, Mat3>;
using EigenSolver4 = EigenSolver<Vec4, Mat4>;

BE_NAMESPACE_END
