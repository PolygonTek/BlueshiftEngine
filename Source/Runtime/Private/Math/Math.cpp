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

#include <cmath>

BE_NAMESPACE_BEGIN

int Math::FloatToBits(float f, int exponentBits, int mantissaBits) {
    assert(exponentBits >= 2 && exponentBits <= 8);
    assert(mantissaBits >= 2 && mantissaBits <= 23);

    int32_t maxBits = (((1 << (exponentBits - 1)) - 1) << mantissaBits) | ((1 << mantissaBits) - 1);
    int32_t minBits = (((1 <<  exponentBits     ) - 2) << mantissaBits) | 1;

    float max = BitsToFloat(maxBits, exponentBits, mantissaBits);
    float min = BitsToFloat(minBits, exponentBits, mantissaBits);

    if (f >= 0.0f) {
        if (f >= max) {
            return maxBits;
        } else if (f <= min) {
            return minBits;
        }
    } else {
        if (f <= -max) {
            return (maxBits | (1 << (exponentBits + mantissaBits)));
        } else if (f >= -min) {
            return (minBits | (1 << (exponentBits + mantissaBits)));
        }
    }

    exponentBits--;
    int32_t i = *reinterpret_cast<int32_t *>(&f);
    int32_t sign = (i >> IEEE_FLT_SIGN_BIT) & 1;
    int32_t exponent = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
    int32_t mantissa = i & ((1 << IEEE_FLT_MANTISSA_BITS) - 1);

    int32_t value = sign << (1 + exponentBits + mantissaBits);
    value |= ((INT32_SIGNBITSET(exponent) << exponentBits) | (Math::Abs(exponent) & ((1 << exponentBits) - 1))) << mantissaBits;
    value |= mantissa >> (IEEE_FLT_MANTISSA_BITS - mantissaBits);
    return value;
}

float Math::BitsToFloat(int i, int exponentBits, int mantissaBits) {
    static int32_t exponentSign[2] = { 1, -1 };

    assert(exponentBits >= 2 && exponentBits <= 8);
    assert(mantissaBits >= 2 && mantissaBits <= 23);

    exponentBits--;
    int32_t sign = i >> (1 + exponentBits + mantissaBits);
    int32_t exponent = ((i >> mantissaBits) & ((1 << exponentBits) - 1)) * exponentSign[(i >> (exponentBits + mantissaBits)) & 1];
    int32_t mantissa = (i & ((1 << mantissaBits) - 1)) << (IEEE_FLT_MANTISSA_BITS - mantissaBits);

    int32_t value = sign << IEEE_FLT_SIGN_BIT | (exponent + IEEE_FLT_EXPONENT_BIAS) << IEEE_FLT_MANTISSA_BITS | mantissa;
    return *reinterpret_cast<float *>(&value);
}

static void SortRoots(double &x1, double &x2) {
    if (x2 < x1) {
        Swap(x1, x2);
    }
}

static void SortRoots(double &x1, double &x2, double &x3) {
    SortRoots(x1, x2);
    SortRoots(x2, x3);
    SortRoots(x1, x2);
}

// a*x^2 + b*x + c = 0 의 서로 다른 실근을 구한다.
static int SolveRealQuadraticInternal(double a, double b, double c, double tolerance, double roots[2]) {
    // 계수에 대한 상대 tolerance 가 방정식의 상수배에 영향받지 않도록 정규화한다.
    const double coefficientScale = AbsMax3(a, b, c);
    if (coefficientScale == 0.0) {
        // 해가 없거나 무한히 많으므로 해를 유일하게 정할 수 없다.
        return 0;
    }

    a /= coefficientScale;
    b /= coefficientScale;
    c /= coefficientScale;

    // a 가 수치적으로 0 이면 선형 방정식으로 처리한다.
    if (std::abs(a) <= tolerance) {
        if (std::abs(b) <= tolerance) {
            return 0;
        }

        roots[0] = -c / b;
        return 1;
    }

    // 판별식은 계수와 스케일이 다르므로 계산 항의 크기를 기준으로 별도 epsilon 을 사용한다.
    const double bSquared = b * b;
    const double fourAC = 4.0 * a * c;
    const double discriminant = bSquared - fourAC;
    const double discriminantTolerance = tolerance * (bSquared + std::abs(fourAC));

    if (discriminant > discriminantTolerance) {
        const double sqrtDiscriminant = Math::Sqrt(discriminant);
        // catastrophic cancellation 이 덜 발생하는 근을 먼저 구하고,
        // 다른 근은 비에타 정리 x1*x2 = c/a 를 이용해서 구한다.
        const double q = -0.5 * (b + std::copysign(sqrtDiscriminant, b));
        roots[0] = q / a;
        roots[1] = c / q;
        SortRoots(roots[0], roots[1]);
        return 2;
    }

    // 중근.
    if (discriminant >= -discriminantTolerance) {
        roots[0] = -0.5 * b / a;
        return 1;
    }

    // 실근이 없다.
    return 0;
}

// a*x^3 + b*x^2 + c*x + d = 0 의 서로 다른 실근을 구한다.
static int SolveRealCubicInternal(double a, double b, double c, double d, double tolerance, double roots[3]) {
    // 계수에 대한 상대 tolerance 가 방정식의 상수배에 영향받지 않도록 정규화한다.
    const double coefficientScale = AbsMax4(a, b, c, d);
    if (coefficientScale == 0.0) {
        return 0;
    }

    a /= coefficientScale;
    b /= coefficientScale;
    c /= coefficientScale;
    d /= coefficientScale;

    // a 가 수치적으로 0 이면 2차 방정식으로 처리한다.
    if (std::abs(a) <= tolerance) {
        return SolveRealQuadraticInternal(b, c, d, tolerance, roots);
    }

    constexpr double OneThird = 1.0 / 3.0;
    constexpr double OneTwentySeventh = 1.0 / 27.0;
    constexpr double SqrtThree = 1.7320508075688772935;

    // 최고차항 계수를 1 로 만들어 monic 다항식으로 변환한다.
    const double inverseA = 1.0 / a;
    const double c2 = b * inverseA;
    const double c1 = c * inverseA;
    const double c0 = d * inverseA;
    const double c2Squared = c2 * c2;

    // x = t - c2/3 을 대입하여 축약된 3차 방정식 t^3 + p*t + q = 0 으로 변환한다.
    // p 와 q 는 스케일이 서로 다르므로 각각의 계산 항을 기준으로 epsilon 을 구한다.
    const double pTerm = OneThird * c2Squared;
    const double p = c1 - pTerm;
    const double pTolerance = tolerance * (std::abs(c1) + std::abs(pTerm));

    const double qCubicTerm = 2.0 * OneTwentySeventh * c2Squared * c2;
    const double qLinearTerm = OneThird * c1 * c2;
    const double q = qCubicTerm - qLinearTerm + c0;
    const double qTolerance = tolerance * (std::abs(qCubicTerm) + std::abs(qLinearTerm) + std::abs(c0));

    // 판별식 delta = (p/3)^3 + (q/2)^2
    const double pOverThree = p * OneThird;
    const double qOverTwo = 0.5 * q;
    const double discriminantP = pOverThree * pOverThree * pOverThree;
    const double discriminantQ = qOverTwo * qOverTwo;
    const double discriminant = discriminantP + discriminantQ;
    const double discriminantTolerance = tolerance * (std::abs(discriminantP) + discriminantQ);
    const double offset = -c2 * OneThird;

    if (discriminant < -discriminantTolerance) {
        // 서로 다른 세 실근. 계산 과정에서 허수를 배제하기 위해
        // atan2 를 이용한 삼각함수형 해법을 사용한다.
        const double theta = Math::ATan2(Math::Sqrt(-discriminant), -qOverTwo);
        const double radius = Math::Sqrt(Max(0.0, -pOverThree));
        const double angle = theta * OneThird;
        double sine;
        double cosine;
        Math::SinCos(angle, sine, cosine);

        roots[0] = offset + 2.0 * radius * cosine;
        roots[1] = offset - radius * (cosine + SqrtThree * sine);
        roots[2] = offset - radius * (cosine - SqrtThree * sine);
        SortRoots(roots[0], roots[1], roots[2]);
        return 3;
    }

    if (discriminant <= discriminantTolerance) {
        // p와 q가 모두 0이면 삼중근이다.
        if (std::abs(p) <= pTolerance && std::abs(q) <= qTolerance) {
            roots[0] = offset;
            return 1;
        }

        // 이중근과 다른 하나의 실근.
        const double repeatedRootTerm = std::cbrt(-qOverTwo);
        roots[0] = offset + 2.0 * repeatedRootTerm;
        roots[1] = offset - repeatedRootTerm;
        SortRoots(roots[0], roots[1]);
        return 2;
    }

    // 하나의 실근과 두 개의 켤레 복소근. 복소근은 무시한다.
    // catastrophic cancellation 을 줄이기 위해 절댓값이 큰 Cardano 피제곱수를 선택하고,
    // 다른 세제곱근은 u*v = -p/3 을 이용해서 구한다.
    const double sqrtDiscriminant = Math::Sqrt(discriminant);
    const double negativeQOverTwo = -qOverTwo;
    const double u = std::cbrt(negativeQOverTwo + std::copysign(sqrtDiscriminant, negativeQOverTwo));
    const double v = -pOverThree / u;
    roots[0] = offset + u + v;
    return 1;
}

int Math::SolveRealQuadratic(float a, float b, float c, float &x1, float &x2, float tolerance) {
    if (!IsFinite(a) || !IsFinite(b) || !IsFinite(c) || tolerance <= 0.0f || !IsFinite(tolerance)) {
        return 0;
    }

    double roots[2];
    const int numRoots = SolveRealQuadraticInternal(a, b, c, tolerance, roots);
    if (numRoots >= 1) {
        x1 = static_cast<float>(roots[0]);
    }
    if (numRoots >= 2) {
        x2 = static_cast<float>(roots[1]);
    }
    return numRoots;
}

int Math::SolveRealCubic(float a, float b, float c, float d, float &x1, float &x2, float &x3, float tolerance) {
    if (!IsFinite(a) || !IsFinite(b) || !IsFinite(c) || !IsFinite(d) || tolerance <= 0.0f || !IsFinite(tolerance)) {
        return 0;
    }

    double roots[3];
    const int numRoots = SolveRealCubicInternal(a, b, c, d, tolerance, roots);
    if (numRoots >= 1) {
        x1 = static_cast<float>(roots[0]);
    }
    if (numRoots >= 2) {
        x2 = static_cast<float>(roots[1]);
    }
    if (numRoots >= 3) {
        x3 = static_cast<float>(roots[2]);
    }
    return numRoots;
}

BE_NAMESPACE_END
