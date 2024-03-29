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

struct BE_API MinMaxCurve {
    struct Type {
        enum Enum {
            Invalid = -1,
            Constant = 0,
            Curve,
            RandomBetweenTwoConstants,
            RandomBetweenTwoCurves
        };
    };

    MinMaxCurve();

    bool                    operator==(const MinMaxCurve &rhs) const;
    bool                    operator!=(const MinMaxCurve &rhs) const;

    void                    Reset(Type::Enum type);
    void                    Reset(Type::Enum type, float scalar, float minValue, float maxValue);

    float                   Evaluate(float random, float t) const;

    float                   Integrate(float random, float t) const;

    static MinMaxCurve      empty;

    Type::Enum              type;
    float                   scalar;
    Hermite<float>          minCurve;
    Hermite<float>          maxCurve;
};

BE_INLINE MinMaxCurve::MinMaxCurve() {
    type = Type::Invalid;
    scalar = 1.0f;
}

BE_INLINE bool MinMaxCurve::operator==(const MinMaxCurve &rhs) const {
    if (type == rhs.type && scalar == rhs.scalar && minCurve == rhs.minCurve && maxCurve == rhs.maxCurve) {
        return true;
    }
    return false;
}

BE_INLINE bool MinMaxCurve::operator!=(const MinMaxCurve &rhs) const {
    if (type != rhs.type || scalar != rhs.scalar || minCurve != rhs.minCurve || maxCurve != rhs.maxCurve) {
        return true;
    }
    return false;
}

BE_INLINE void MinMaxCurve::Reset(Type::Enum type) {
    this->type = type;
    this->scalar = 1.0f;
    this->minCurve.Clear();
    this->maxCurve.Clear();
}

BE_INLINE void MinMaxCurve::Reset(Type::Enum type, float scalar, float minValue, float maxValue) {
    assert(minValue >= -1.0f && minValue <= 1.0f);
    assert(maxValue >= -1.0f && maxValue <= 1.0f);
    this->type = type;
    this->scalar = scalar;
    this->minCurve.Clear();
    this->minCurve.AddPoint(0, minValue);
    this->maxCurve.Clear();
    this->maxCurve.AddPoint(0, maxValue);
}

BE_INLINE float MinMaxCurve::Evaluate(float random, float t) const {
    switch (type) {
    case Type::Constant:
        return scalar * maxCurve.GetPoint(0);
    case Type::Curve:
        return scalar * maxCurve.Evaluate(t);
    case Type::RandomBetweenTwoConstants:
        return scalar * Math::Lerp(minCurve.GetPoint(0), maxCurve.GetPoint(0), random);
    case Type::RandomBetweenTwoCurves:
        return scalar * Math::Lerp(minCurve.Evaluate(t), maxCurve.Evaluate(t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_INLINE float MinMaxCurve::Integrate(float random, float t) const {
    switch (type) {
    case Type::Constant:
        return scalar * maxCurve.GetPoint(0) * t;
    case Type::Curve:
        return scalar * maxCurve.Integrate(0, t);
    case Type::RandomBetweenTwoConstants:
        return scalar * Math::Lerp(minCurve.GetPoint(0), maxCurve.GetPoint(0), random) * t;
    case Type::RandomBetweenTwoCurves:
        return scalar * Math::Lerp(minCurve.Integrate(0, t), maxCurve.Integrate(0, t), random);
    default:
        assert(0);
        return 0;
    }
}

BE_NAMESPACE_END
