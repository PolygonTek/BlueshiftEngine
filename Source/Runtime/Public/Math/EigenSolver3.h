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

BE_NAMESPACE_BEGIN

struct BE_API EigenPair3 {
    float               eigenValue;
    Vec3                eigenVector;
    float               residual;
    int                 numIterations;
};

namespace EigenSolver3 {
    /// Finds the real eigenpair with the largest eigenvalue magnitude using power iteration.
    /// The result is left unchanged if the iteration fails to converge.
    BE_API bool PowerIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations = 32, float tolerance = MATRIX_EPSILON);

    /// Finds the real eigenpair nearest to shift using shifted inverse power iteration.
    /// The shifted matrix is factored once and solved without explicitly forming its inverse.
    /// The result is left unchanged if factorization or iteration fails.
    BE_API bool InversePowerIteration(const Mat3 &matrix, float shift, const Vec3 &initialVector, EigenPair3 &result, int maxIterations = 32, float tolerance = MATRIX_EPSILON);

    /// Refines an eigenpair of a real symmetric matrix using Rayleigh quotient iteration.
    /// The shift is recomputed from the current vector on every iteration.
    /// The result is left unchanged if validation, factorization, or iteration fails.
    BE_API bool RayleighQuotientIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations = 16, float tolerance = MATRIX_EPSILON);
}

BE_NAMESPACE_END
