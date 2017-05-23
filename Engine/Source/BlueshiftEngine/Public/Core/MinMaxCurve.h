// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

struct MinMaxCurve {
    enum Type {
        ConstantType,
        CurveType,
        RandomBetweenTwoConstantsType,
        RandomBetweenTwoCurvesType
    };

    void                    Reset();
    void                    Reset(float s, float minValue, float maxValue);

    float                   Evaluate(float random, float t) const;

    float                   Integrate(float random, float t) const;

    Type                    type;
    float                   scalar;
    Hermite<float>          minCurve;
    Hermite<float>          maxCurve;
};

BE_INLINE void MinMaxCurve::Reset() {
    type = ConstantType;
    scalar = 1.0f;
    minCurve.Clear();
    maxCurve.Clear();
}

BE_INLINE void MinMaxCurve::Reset(float s, float minValue, float maxValue) {
    assert(minValue >= -1.0f && minValue <= 1.0f);
    assert(maxValue >= -1.0f && maxValue <= 1.0f);
    type = ConstantType;
    scalar = s;
    minCurve.Clear();
    minCurve.AddPoint(0, minValue);
    maxCurve.Clear();
    maxCurve.AddPoint(0, maxValue);
}

BE_INLINE float MinMaxCurve::Evaluate(float random, float t) const {
    switch (type) {
    case ConstantType:
        return scalar * maxCurve.GetPoint(0);
    case CurveType:
        return scalar * maxCurve.Evaluate(t);
    case RandomBetweenTwoConstantsType:
        return scalar * Lerp(minCurve.GetPoint(0), maxCurve.GetPoint(0), random);
    case RandomBetweenTwoCurvesType:
        return scalar * Lerp(minCurve.Evaluate(t), maxCurve.Evaluate(t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_INLINE float MinMaxCurve::Integrate(float random, float t) const {
    switch (type) {
    case ConstantType:
        return scalar * maxCurve.GetPoint(0) * t;
    case CurveType:
        return scalar * maxCurve.Integrate(0, t);
    case RandomBetweenTwoConstantsType:
        return scalar * Lerp(minCurve.GetPoint(0), maxCurve.GetPoint(0), random) * t;
    case RandomBetweenTwoCurvesType:
        return scalar * Lerp(minCurve.Integrate(0, t), maxCurve.Integrate(0, t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_NAMESPACE_END
