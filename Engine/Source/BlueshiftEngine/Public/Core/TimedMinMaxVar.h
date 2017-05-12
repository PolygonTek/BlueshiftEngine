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

struct MinMaxVar {
    enum Type {
        ConstantType,
        RandomBetweenTwoConstantsType
    };

    void                    Reset();

    float                   Evaluate(float random) const;

    Type                    type;
    float                   constants[2];
};

BE_INLINE void MinMaxVar::Reset() {
    type = ConstantType;
    constants[0] = 0.0f;
    constants[1] = 0.0f;
}

BE_INLINE float MinMaxVar::Evaluate(float random) const {
    switch (type) {
    case ConstantType:
        return constants[1];
    case RandomBetweenTwoConstantsType:
        return Lerp(constants[0], constants[1], random);
    default:
        assert(0);
        return 0;
    }
}

struct TimedVar {
    enum Type {
        ConstantType,
        CurveType
    };

    void                    Reset();

    float                   Evaluate(float t) const;

    float                   Integrate(float t) const;

    Type                    type;
    float                   constant;
    Hermite<float>          curve;
};

BE_INLINE void TimedVar::Reset() {
    type = ConstantType;
    constant = 0.0f;
    curve.Clear();
}

BE_INLINE float TimedVar::Evaluate(float t) const {
    switch (type) {
    case ConstantType:
        return constant;
    case CurveType:
        return constant * curve.Evaluate(t);
    default:
        assert(0);
        return 0;
    }
}

BE_INLINE float TimedVar::Integrate(float t) const {
    switch (type) {
    case ConstantType:
        return constant * t;
    case CurveType:
        return constant * curve.Integrate(0, t);
    default:
        assert(0);
        return 0;
    }
}

struct TimedMinMaxVar {
    enum Type {
        ConstantType,
        CurveType,
        RandomBetweenTwoConstantsType,
        RandomBetweenTwoCurvesType
    };

    void                    Reset();

    float                   Evaluate(float random, float t) const;

    float                   Integrate(float random, float t) const;

    Type                    type;
    float                   constants[2];
    Hermite<float>          curves[2];
};

BE_INLINE void TimedMinMaxVar::Reset() {
    type = ConstantType;
    constants[0] = 0.0f;
    constants[1] = 0.0f;
    curves[0].Clear();
    curves[1].Clear();
}

BE_INLINE float TimedMinMaxVar::Evaluate(float random, float t) const {
    switch (type) {
    case ConstantType:
        return constants[1];
    case CurveType:
        return constants[1] * curves[1].Evaluate(t);
    case RandomBetweenTwoConstantsType:
        return Lerp(constants[0], constants[1], random);
    case RandomBetweenTwoCurvesType:
        return Lerp(constants[0] * curves[0].Evaluate(t), constants[1] * curves[1].Evaluate(t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_INLINE float TimedMinMaxVar::Integrate(float random, float t) const {
    switch (type) {
    case ConstantType:
        return constants[1] * t;
    case CurveType:
        return constants[1] * curves[1].Integrate(0, t);
    case RandomBetweenTwoConstantsType:
        return Lerp(constants[0], constants[1], random) * t;
    case RandomBetweenTwoCurvesType:
        return Lerp(constants[0] * curves[0].Integrate(0, t), constants[1] * curves[1].Integrate(0, t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_NAMESPACE_END
