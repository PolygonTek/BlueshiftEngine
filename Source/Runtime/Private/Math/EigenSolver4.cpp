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

bool EigenSolver4::PowerIteration(const Mat4 &matrix, const Vec4 &initialVector, EigenPair4 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::PowerIteration<4>(matrix, initialVector, result, maxIterations, tolerance);
}

bool EigenSolver4::InversePowerIteration(const Mat4 &matrix, float shift, const Vec4 &initialVector, EigenPair4 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::InversePowerIteration<4>(matrix, shift, initialVector, result, maxIterations, tolerance);
}

bool EigenSolver4::RayleighQuotientIteration(const Mat4 &matrix, const Vec4 &initialVector, EigenPair4 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::RayleighQuotientIteration<4>(matrix, initialVector, result, maxIterations, tolerance);
}

BE_NAMESPACE_END
