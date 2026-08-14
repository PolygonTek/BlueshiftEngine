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

bool EigenSolver3::PowerIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::PowerIteration<3>(matrix, initialVector, result, maxIterations, tolerance);
}

bool EigenSolver3::InversePowerIteration(const Mat3 &matrix, float shift, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::InversePowerIteration<3>(matrix, shift, initialVector, result, maxIterations, tolerance);
}

bool EigenSolver3::RayleighQuotientIteration(const Mat3 &matrix, const Vec3 &initialVector, EigenPair3 &result, int maxIterations, float tolerance) {
    return EigenSolverFixed::RayleighQuotientIteration<3>(matrix, initialVector, result, maxIterations, tolerance);
}

BE_NAMESPACE_END
