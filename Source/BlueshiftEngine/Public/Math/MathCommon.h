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

#define INT8_SIGN_BIT               7
#define INT16_SIGN_BIT              15
#define INT32_SIGN_BIT              31
#define INT64_SIGN_BIT              63

#define INT8_SIGN_MASK              (1 << INT8_SIGN_BIT)
#define INT16_SIGN_MASK             (1 << INT16_SIGN_BIT)
#define INT32_SIGN_MASK             (1UL << INT32_SIGN_BIT)
#define INT64_SIGN_MASK             (1ULL << INT64_SIGN_BIT)

// int 비트 검출
#define INT64_SIGNBITSET(i)         (((const uint64_t)(i)) >> INT64_SIGN_BIT)
#define INT64_SIGNBITNOTSET(i)      ((~((const uint64_t)(i))) >> INT64_SIGN_BIT)
#define INT32_SIGNBITSET(i)         (((const uint32_t)(i)) >> INT32_SIGN_BIT)
#define INT32_SIGNBITNOTSET(i)      ((~((const uint32_t)(i))) >> INT32_SIGN_BIT)
#define INT16_SIGNBITSET(i)         (((const uint16_t)(i)) >> INT16_SIGN_BIT)
#define INT16_SIGNBITNOTSET(i)      ((~((const uint16_t)(i))) >> INT16_SIGN_BIT)

/*
-------------------------------------------------------------------------------

floating point bit layouts according to the IEEE 754-1985 and 754-2008 standard

-------------------------------------------------------------------------------
*/

// IEEE half float - 16 bit
#define IEEE_FLT16_MANTISSA_BITS    10
#define IEEE_FLT16_EXPONENT_BITS    5
#define IEEE_FLT16_EXPONENT_BIAS    15
#define IEEE_FLT16_SIGN_BIT         15

// IEEE single float - 32 bit
#define IEEE_FLT_MANTISSA_BITS      23
#define IEEE_FLT_EXPONENT_BITS      8
#define IEEE_FLT_EXPONENT_BIAS      127
#define IEEE_FLT_SIGN_BIT           31

// IEEE double - 64 bit
#define IEEE_DBL_MANTISSA_BITS      52
#define IEEE_DBL_EXPONENT_BITS      11
#define IEEE_DBL_EXPONENT_BIAS      1023
#define IEEE_DBL_SIGN_BIT           63

// IEEE double extended - 80 bit
#define IEEE_DBLE_MANTISSA_BITS     64
#define IEEE_DBLE_EXPONENT_BITS     15
#define IEEE_DBLE_EXPONENT_BIAS     16383
#define IEEE_DBLE_SIGN_BIT          79

// IEEE quad double - 128 bit
#define IEEE_QDBL_MANTISSA_BITS     112
#define IEEE_QDBL_EXPONENT_BITS     15
#define IEEE_QDBL_EXPONENT_BIAS     16383
#define IEEE_QDBL_SIGN_BIT          127

// float 비트 검출
#define IEEE_FLT_SIGNBITSET(f)      ((*(const uint32_t *)&(f)) >> 31)
#define IEEE_FLT_SIGNBITNOTSET(f)   ((~(*(const uint32_t *)&(f))) >> 31)
#define IEEE_FLT_NOTZERO(f)         ((*(const uint32_t *)&(f)) & ~(1 << 31))

// float NaN, INF, IND, DENORMAL 검출 (INF, IND 는 NaN 에 포함)
#define IEEE_FLT_IS_NAN(x)          (((*(const uint32_t *)&x) & 0x7f800000) == 0x7f800000)
#define IEEE_FLT_IS_INF(x)          (((*(const uint32_t *)&x) & 0x7fffffff) == 0x7f800000)
#define IEEE_FLT_IS_IND(x)          ((*(const uint32_t *)&x) == 0xffc00000)
#define IEEE_FLT_IS_DENORMAL(x)     (((*(const uint32_t *)&x) & 0x7f800000) == 0x00000000 && \
                                    ((*(const uint32_t *)&x) & 0x007fffff) != 0x00000000)
// degree <-> radian
#define DEG2RAD(a)                  ((a) * BE1::Math::MulDegreeToRadian)
#define RAD2DEG(a)                  ((a) * BE1::Math::MulRadianToDegree)

// seconds <-> miliseconds
#define SEC2MS(t)                   (BE1::Math::FtoiFast((t) * BE1::Math::MulSecondToMs))
#define MS2SEC(t)                   ((t) * BE1::Math::MulMsToSecond)

// degree <-> short value
#define ANGLE2SHORT(x)              (BE1::Math::FtoiFast((x) * 65536.0f / 360.0f) & 65535)
#define SHORT2ANGLE(x)              ((x) * (360.0f / 65536.0f))

// degree <-> byte value            
#define ANGLE2BYTE(x)               (BE1::Math::FtoiFast((x) * 256.0f / 360.0f) & 255)
#define BYTE2ANGLE(x)               ((x) * (360.0f / 256.0f))

#define C_FLOAT_TO_INT(x)           (int)(x)

BE_NAMESPACE_BEGIN

template <typename T> BE_INLINE T   Sign(const T v) { return (v > 0) ? 1 : ((v < 0) ? -1 : 0 ); }
template <typename T> BE_INLINE T   Square(const T &v) { return v * v; }
template <typename T> BE_INLINE T   Cube(const T &v) { return v * v * v; }

template <typename T> BE_INLINE T   InchesToMetres(const T value) { return static_cast<T>(value * 0.0254f); }
template <typename T> BE_INLINE T   MetresToInches(const T value) { return static_cast<T>(value * 39.37f); }
template <typename T> BE_INLINE T   InchesToFeet(const T value) { return static_cast<T>(value / 12.f); }
template <typename T> BE_INLINE T   FeetToMiles(const T value) { return static_cast<T>(value / 5280.f); }
template <typename T> BE_INLINE T   FeetToInches(const T value) { return static_cast<T>(value * 12.f); }
template <typename T> BE_INLINE T   MetresToFeet(const T value) { return InchesToFeet(MetresToInches(value)); }
template <typename T> BE_INLINE T   FeetToMetres(const T value) { return FeetToInches(InchesToMetres(value)); }

template <unsigned int Value>
struct Factorial {
    enum { answer = Value * Factorial<Value - 1>::answer };
};
 
template<>
struct Factorial<0> {
    enum { answer = 1 };
};

class BE_API Math {
public:
    static const float          Pi;                         ///< pi
    static const float          TwoPi;                      ///< pi * 2
    static const float          HalfPi;                     ///< pi / 2
    static const float          OneFourthPi;                ///< pi / 4
    static const float          InvPi;                      ///< 1 / pi
    static const float          InvTwoPi;                   ///< 1 / 2pi
    static const float          E;                          ///< e
    static const float          GoldenRatio;                ///< 1 / 1.61803398875
    static const float          SqrtTwo;                    ///< sqrt( 2 )
    static const float          SqrtThree;                  ///< sqrt( 3 )
    static const float          SqrtOneOverTwo;             ///< sqrt( 1 / 2 )
    static const float          SqrtOneOverThree;           ///< sqrt( 1 / 3 )
    static const float          MulDegreeToRadian;          ///< degrees to radians multiplier
    static const float          MulRadianToDegree;          ///< radians to degrees multiplier
    static const float          MulSecondToMs;              ///< seconds to milliseconds multiplier
    static const float          MulMsToSecond;              ///< milliseconds to seconds multiplier
    static const float          Infinity;                   ///< huge number which should be larger than any valid number used
    static const float          FloatEpsilon;               ///< smallest positive number such that 1.0 + FloatEpsilon != 1.0
    static const float          FloatSmallestNonDenormal;   ///< smallest non-denormal 32-bit floating point value
    
#if defined(BE_WIN_X86_SSE_INTRIN)
    static const __m128         SIMD_SP_zero;
    static const __m128         SIMD_SP_255;
    static const __m128         SIMD_SP_min_char;
    static const __m128         SIMD_SP_max_char;
    static const __m128         SIMD_SP_min_short;
    static const __m128         SIMD_SP_max_short;
    static const __m128         SIMD_SP_smallestNonDenorm;
    static const __m128         SIMD_SP_tiny;
    static const __m128         SIMD_SP_rsqrt_c0;
    static const __m128         SIMD_SP_rsqrt_c1;
#endif

    static void                 Init();

                                /// Reciprocal square root, returns huge number when x == 0.0
    static float                RSqrt(float x);

                                /// Inverse square root with 32 bits precision, returns huge number when x == 0.0
    static float                InvSqrt(float x);
                                /// Inverse square root with 16 bits precision, returns huge number when x == 0.0
    static float                InvSqrt16(float x);
                                /// Inverse square root with 64 bits precision, returns huge number when x == 0.0
    static double               InvSqrt64(float x);

                                /// Square root with 32 bits precision
    static float                Sqrt(float x);
                                /// Square root with 16 bits precision
    static float                Sqrt16(float x);
                                /// Square root with 64 bits precision
    static double               Sqrt64(float x);

                                /// Sine with 32 bits precision
    static float                Sin(float a);
                                /// Sine with 16 bits precision, maximum absolute error is 2.3082e-09
    static float                Sin16(float a);
                                /// Sine with 64 bits precision
    static double               Sin64(float a);

                                /// Cosine with 32 bits precision
    static float                Cos(float a);
                                /// Cosine with 16 bits precision, maximum absolute error is 2.3082e-09
    static float                Cos16(float a);
                                /// Cosine with 64 bits precision
    static double               Cos64(float a);

                                /// Sine and cosine with 32 bits precision
    static void                 SinCos(float a, float &s, float &c);
                                /// Sine and cosine with 16 bits precision
    static void                 SinCos16(float a, float &s, float &c);
                                /// Sine and cosine with 64 bits precision
    static void                 SinCos64(float a, double &s, double &c);

                                /// Tangent with 32 bits precision
    static float                Tan(float a);
                                /// Tangent with 16 bits precision, maximum absolute error is 1.8897e-08
    static float                Tan16(float a);
                                /// Tangent with 64 bits precision
    static double               Tan64(float a);

                                /// Arc sine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
    static float                ASin(float a);
                                /// Arc sine with 16 bits precision, maximum absolute error is 6.7626e-05
    static float                ASin16(float a);
                                /// Arc sine with 64 bits precision
    static double               ASin64(float a);

                                /// Arc cosine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
    static float                ACos(float a);
                                /// Arc cosine with 16 bits precision, maximum absolute error is 6.7626e-05
    static float                ACos16(float a);
                                /// Arc cosine with 64 bits precision
    static double               ACos64(float a);

                                /// Arc tangent with 32 bits precision
    static float                ATan(float a);
                                /// Arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08
    static float                ATan16(float a);
                                /// Arc tangent with 64 bits precision
    static double               ATan64(float a);

                                /// Arc tangent with 32 bits precision
    static float                ATan(float y, float x);
                                /// Arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08
    static float                ATan16(float y, float x);
                                /// Arc tangent with 64 bits precision
    static double               ATan64(float y, float x);

                                /// x raised to the power y with 32 bits precision
    static float                Pow(float x, float y);
                                /// x raised to the power y with 16 bits precision
    static float                Pow16(float x, float y);
                                /// x raised to the power y with 64 bits precision
    static double               Pow64(float x, float y);

                                /// e raised to the power f with 32 bits precision
    static float                Exp(float f);
                                /// e raised to the power f with 16 bits precision
    static float                Exp16(float f);
                                /// e raised to the power f with 64 bits precision
    static double               Exp64(float f);

                                /// Natural logarithm with 32 bits precision
    static float                Log(float f);
                                /// Natural logarithm with 16 bits precision
    static float                Log16(float f);
                                /// Natural logarithm with 64 bits precision
    static double               Log64(float f);

                                /// Base b logarithm with 32 bits precision
    static float                Log(float b, float f);
                                /// Base b logarithm with 16 bits precision
    static float                Log16(float b, float f);
                                /// Base b logarithm with 64 bits precision
    static double               Log64(float b, float f);

                                /// Integral x raised to the power y
    static int                  IPow(int x, int y);
                                /// Integral base-2 logarithm of the floating point value
    static int                  ILog2(float f);
                                /// Integral base-2 logarithm of the integer value
    static int                  ILog2(int i);

                                /// Minumum number of bits required to represent ceil(f)
    static int                  BitsForFloat(float f);
                                /// Minumum number of bits required to represent i
    static int                  BitsForInteger(int i);
                                /// Round x down to the nearest power of 2
    static int                  FloorPowerOfTwo(int x);
                                /// Round x up to the nearest power of 2
    static int                  CeilPowerOfTwo(int x);
                                /// Returns true if x is a power of 2
    static bool                 IsPowerOfTwo(int x);

                                /// Returns the absolute value of the integer value (for reference only)
    static int                  Abs(int x);
                                /// Returns the absolute value of the floating point value
    static float                Fabs(float f);
                                /// Returns the largest integer that is less than or equal to the given value
    static float                Floor(float f);
                                /// Returns the smallest integer that is greater than or equal to the given value
    static float                Ceil(float f);
                                /// Returns the fraction component (part after the decimal)
    static float                Fract(float f);
                                /// Returns the nearest integer
    static float                Rint(float f);
                                /// Float to int conversion
    static int                  Ftoi(float f);
                                /// Fast float to int conversion but uses current FPU round mode (default round nearest)
    static int                  FtoiFast(float f);
                                /// Float to char conversion
    static int8_t               Ftoi8(float f);
                                /// Float to short conversion
    static int16_t              Ftoi16(float f);
                                /// Float to unsigned short conversion
    static uint16_t             Ftoui16(float f);
                                /// Float to byte conversion, the result is clamped to the range [0-255]
    static byte                 Ftob(float f);

    static double               Factorial(unsigned int n);

    static float                FloorSnap(float value, float snapSize);

    static float                CeilSnap(float value, float snapSize);

    static float                AngleNormalize360(float angle);
    static float                AngleNormalize180(float angle);
    static float                AngleDelta(float angle1, float angle2);

    static int                  FloatToBits(float f, int exponentBits, int mantissaBits);
    static float                BitsToFloat(int i, int exponentBits, int mantissaBits);

    static int                  FloatHash(const float *arr, const int numFloats);

    static float                Random(float minimum, float maximum);

private:
    enum {
        LOOKUP_BITS             = 8,
        EXP_POS                 = 23,
        EXP_BIAS                = 127,
        LOOKUP_POS              = (EXP_POS-LOOKUP_BITS), 
        SEED_POS                = (EXP_POS-8), 
        SQRT_TABLE_SIZE         = (2<<LOOKUP_BITS), 
        LOOKUP_MASK             = (SQRT_TABLE_SIZE-1)
    };

    union _flint {
        uint32_t                i;
        float                   f;
    };

    static uint32_t             iSqrt[SQRT_TABLE_SIZE];
    static bool                 initialized;
};

BE_INLINE float Math::RSqrt(float x) {
    int32_t i;
    float y, r;

    y = x * 0.5f;
    i = *reinterpret_cast<int32_t *>(&x);
    i = 0x5f3759df - (i >> 1);
    r = *reinterpret_cast<float *>(&i);
    r = r * (1.5f - r * r * y);
    return r;
}

BE_INLINE float Math::InvSqrt16(float x) {
    uint32_t a = ((union _flint*)(&x))->i;
    union _flint seed;

    assert(initialized);

    double y = x * 0.5f;
    seed.i = ((((3*EXP_BIAS-1) - ((a >> EXP_POS) & 0xFF)) >> 1)<<EXP_POS) | iSqrt[(a >> (EXP_POS-LOOKUP_BITS)) & LOOKUP_MASK];
    double r = seed.f;
    r = r * (1.5f - r * r * y);
    return (float)r;
}

BE_INLINE float Math::InvSqrt(float x) {
    uint32_t a = ((union _flint*)(&x))->i;
    union _flint seed;

    assert(initialized);

    double y = x * 0.5f;
    seed.i = ((((3*EXP_BIAS-1) - ((a >> EXP_POS) & 0xFF)) >> 1)<<EXP_POS) | iSqrt[(a >> (EXP_POS-LOOKUP_BITS)) & LOOKUP_MASK];
    double r = seed.f;
    r = r * (1.5f - r * r * y);
    r = r * (1.5f - r * r * y);
    return (float) r;
}

BE_INLINE double Math::InvSqrt64(float x) {
    uint32_t a = ((union _flint*)(&x))->i;
    union _flint seed;

    assert(initialized);

    double y = x * 0.5f;
    seed.i = ((((3*EXP_BIAS-1) - ((a >> EXP_POS) & 0xFF)) >> 1)<<EXP_POS) | iSqrt[(a >> (EXP_POS-LOOKUP_BITS)) & LOOKUP_MASK];
    double r = seed.f;
    r = r * (1.5f - r * r * y);
    r = r * (1.5f - r * r * y);
    r = r * (1.5f - r * r * y);
    return r;
}

BE_INLINE float Math::Sqrt16(float x) {
    return x * InvSqrt16(x);
}

BE_INLINE float Math::Sqrt(float x) {
    return x * InvSqrt(x);
}

BE_INLINE double Math::Sqrt64(float x) {
    return x * InvSqrt64(x);
}

BE_INLINE float Math::Sin(float a) {
    return sinf(a);
}

BE_INLINE float Math::Sin16(float a) {
    float s;

    if ((a < 0.0f) || (a >= TwoPi)) {
        a -= floorf(a * InvTwoPi) * TwoPi;
    }
#if 1
    if (a < Pi) {
        if (a > HalfPi) {
            a = Pi - a;
        }
    } else {
        if (a > Pi + HalfPi) {
            a = a - TwoPi;
        } else {
            a = Pi - a;
        }
    }
#else
    a = Pi - a;
    if (fabs(a) >= HalfPi) {
        a = ((a < 0.0f) ? -Pi : Pi) - a;
    }
#endif
    s = a * a;
    return a * (((((-2.39e-08f * s + 2.7526e-06f) * s - 1.98409e-04f) * s + 8.3333315e-03f) * s - 1.666666664e-01f) * s + 1.0f);
}

BE_INLINE double Math::Sin64(float a) {
    return sin(a);
}

BE_INLINE float Math::Cos(float a) {
    return cosf(a);
}

BE_INLINE float Math::Cos16(float a) {
    float s, d;

    if ((a < 0.0f) || (a >= TwoPi)) {
        a -= floorf(a * InvTwoPi) * TwoPi;
    }
#if 1
    if (a < Pi) {
        if (a > HalfPi) {
            a = Pi - a;
            d = -1.0f;
        } else {
            d = 1.0f;
        }
    } else {
        if (a > Pi + HalfPi) {
            a = a - TwoPi;
            d = 1.0f;
        } else {
            a = Pi - a;
            d = -1.0f;
        }
    }
#else
    a = Pi - a;
    if (fabs(a) >= HalfPi) {
        a = ((a < 0.0f) ? -Pi : Pi) - a;
        d = 1.0f;
    } else {
        d = -1.0f;
    }
#endif
    s = a * a;
    return d * (((((-2.605e-07f * s + 2.47609e-05f) * s - 1.3888397e-03f) * s + 4.16666418e-02f) * s - 4.999999963e-01f) * s + 1.0f);
}

BE_INLINE double Math::Cos64(float a) {
    return cos(a);
}

BE_INLINE void Math::SinCos(float a, float &s, float &c) {
#if defined(__WIN32__) && !defined(__X86_64__)
    _asm {
        fld     a
        fsincos
        mov     ecx, c
        mov     edx, s
        fstp    dword ptr [ecx]
        fstp    dword ptr [edx]
    }
#else
    s = sinf(a);
    c = cosf(a);
#endif
}

BE_INLINE void Math::SinCos16(float a, float &s, float &c) {
    float t, d;

    if ((a < 0.0f) || (a >= Math::TwoPi)) {
        a -= floorf(a / Math::TwoPi) * Math::TwoPi;
    }
#if 1
    if (a < Pi) {
        if (a > HalfPi) {
            a = Pi - a;
            d = -1.0f;
        } else {
            d = 1.0f;
        }
    } else {
        if (a > Pi + HalfPi) {
            a = a - TwoPi;
            d = 1.0f;
        } else {
            a = Pi - a;
            d = -1.0f;
        }
    }
#else
    a = Pi - a;
    if (fabs(a) >= HalfPi) {
        a = ((a < 0.0f) ? -Pi : Pi) - a;
        d = 1.0f;
    } else {
        d = -1.0f;
    }
#endif
    t = a * a;
    s = a * (((((-2.39e-08f * t + 2.7526e-06f) * t - 1.98409e-04f) * t + 8.3333315e-03f) * t - 1.666666664e-01f) * t + 1.0f);
    c = d * (((((-2.605e-07f * t + 2.47609e-05f) * t - 1.3888397e-03f) * t + 4.16666418e-02f) * t - 4.999999963e-01f) * t + 1.0f);
}

BE_INLINE void Math::SinCos64(float a, double &s, double &c) {
#if defined(__WIN32__) && !defined(__X86_64__)
    _asm {
        fld		a
        fsincos
        mov		ecx, c
        mov		edx, s
        fstp	qword ptr [ecx]
        fstp	qword ptr [edx]
    }
#else
    s = sin(a);
    c = cos(a);
#endif
}

BE_INLINE float Math::Tan(float a) {
    return tanf(a);
}

BE_INLINE float Math::Tan16(float a) {
    float s;
    bool reciprocal;

    if ((a < 0.0f) || (a >= Pi)) {
        a -= floorf( a / Pi ) * Pi;
    }
#if 1
    if (a < HalfPi) {
        if (a > OneFourthPi) {
            a = HalfPi - a;
            reciprocal = true;
        } else {
            reciprocal = false;
        }
    } else {
        if (a > HalfPi + OneFourthPi) {
            a = a - Pi;
            reciprocal = false;
        } else {
            a = HalfPi - a;
            reciprocal = true;
        }
    }
#else
    a = HalfPi - a;
    if (fabs(a) >= OneFourthPi) {
        a = ((a < 0.0f) ? -HalfPi : HalfPi) - a;
        reciprocal = false;
    } else {
        reciprocal = true;
    }
#endif
    s = a * a;
    s = a * ((((((9.5168091e-03f * s + 2.900525e-03f) * s + 2.45650893e-02f) * s + 5.33740603e-02f) * s + 1.333923995e-01f) * s + 3.333314036e-01f) * s + 1.0f);
    if (reciprocal) {
        return 1.0f / s;
    } else {
        return s;
    }
}

BE_INLINE double Math::Tan64(float a) {
    return tan(a);
}

BE_INLINE float Math::ASin(float a) {
    if (a <= -1.0f) {
        return -HalfPi;
    }
    if (a >= 1.0f) {
        return HalfPi;
    }
    return asinf(a);
}

BE_INLINE float Math::ASin16(float a) {
    if (IEEE_FLT_SIGNBITSET(a)) {
        if (a <= -1.0f) {
            return -HalfPi;
        }
        a = fabs(a);
        return (((-0.0187293f * a + 0.0742610f) * a - 0.2121144f) * a + 1.5707288f) * sqrt(1.0f - a) - HalfPi;
    } else {
        if (a >= 1.0f) {
            return HalfPi;
        }
        return HalfPi - (((-0.0187293f * a + 0.0742610f) * a - 0.2121144f) * a + 1.5707288f) * sqrt(1.0f - a);
    }
}

BE_INLINE double Math::ASin64(float a) {
    if (a <= -1.0f) {
        return -HalfPi;
    }
    if (a >= 1.0f) {
        return HalfPi;
    }
    return asin(a);
}

BE_INLINE float Math::ACos(float a) {
    if (a <= -1.0f) {
        return Pi;
    }
    if (a >= 1.0f) {
        return 0.0f;
    }
    return acosf(a);
}

BE_INLINE float Math::ACos16(float a) {
    if (IEEE_FLT_SIGNBITSET(a)) {
        if (a <= -1.0f) {
            return Pi;
        }
        a = fabs(a);
        return Pi - (((-0.0187293f * a + 0.0742610f) * a - 0.2121144f) * a + 1.5707288f) * sqrt(1.0f - a);
    } else {
        if (a >= 1.0f) {
            return 0.0f;
        }
        return (((-0.0187293f * a + 0.0742610f) * a - 0.2121144f) * a + 1.5707288f) * sqrt(1.0f - a);
    }
}

BE_INLINE double Math::ACos64(float a) {
    if (a <= -1.0f) {
        return Pi;
    }
    if (a >= 1.0f) {
        return 0.0f;
    }
    return acos(a);
}

BE_INLINE float Math::ATan(float a) {
    return atanf(a);
}

BE_INLINE float Math::ATan16(float a) {
    float s;

    if (fabs(a) > 1.0f) {
        a = 1.0f / a;
        s = a * a;
        s = - (((((((((0.0028662257f * s - 0.0161657367f) * s + 0.0429096138f) * s - 0.0752896400f)
                * s + 0.1065626393f) * s - 0.1420889944f) * s + 0.1999355085f) * s - 0.3333314528f) * s) + 1.0f) * a;
        if (IEEE_FLT_SIGNBITSET(a)) {
            return s - HalfPi;
        } else {
            return s + HalfPi;
        }
    } else {
        s = a * a;
        return (((((((((0.0028662257f * s - 0.0161657367f) * s + 0.0429096138f) * s - 0.0752896400f)
            * s + 0.1065626393f) * s - 0.1420889944f) * s + 0.1999355085f) * s - 0.3333314528f) * s) + 1.0f) * a;
    }
}

BE_INLINE double Math::ATan64(float a) {
    return atan(a);
}

BE_INLINE float Math::ATan(float y, float x) {
    return atan2f(y, x);
}

BE_INLINE float Math::ATan16(float y, float x) {
    float a, s;

    if (fabs(y) > fabs(x)) {
        a = x / y;
        s = a * a;
        s = - (((((((((0.0028662257f * s - 0.0161657367f) * s + 0.0429096138f) * s - 0.0752896400f)
                * s + 0.1065626393f) * s - 0.1420889944f) * s + 0.1999355085f) * s - 0.3333314528f) * s) + 1.0f) * a;
        if (IEEE_FLT_SIGNBITSET(a)) {
            return s - HalfPi;
        } else {
            return s + HalfPi;
        }
    } else {
        a = y / x;
        s = a * a;
        return (((((((((0.0028662257f * s - 0.0161657367f) * s + 0.0429096138f) * s - 0.0752896400f)
            * s + 0.1065626393f) * s - 0.1420889944f) * s + 0.1999355085f) * s - 0.3333314528f) * s) + 1.0f) * a;
    }
}

BE_INLINE double Math::ATan64(float y, float x) {
    return atan2(y, x);
}

BE_INLINE float Math::Pow(float x, float y) {
    return powf(x, y);
}

BE_INLINE float Math::Pow16(float x, float y) {
    return Exp16(y * Log16(x));
}

BE_INLINE double Math::Pow64(float x, float y) {
    return pow(x, y);
}

BE_INLINE float Math::Exp(float f) {
    return expf(f);
}

BE_INLINE float Math::Exp16(float f) {
    float x = f * 1.44269504088896340f;		// multiply with ( 1 / log( 2 ) )
#if 1
    int i = *reinterpret_cast<int *>(&x);
    int s = (i >> IEEE_FLT_SIGN_BIT);
    int e = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
    int m = (i & ((1 << IEEE_FLT_MANTISSA_BITS) - 1)) | (1 << IEEE_FLT_MANTISSA_BITS);
    i = ((m >> (IEEE_FLT_MANTISSA_BITS - e)) & ~(e >> 31)) ^ s;
#else
    i = (int)x;
    if (x < 0.0f)
        i--;
#endif
    int exponent = (i + IEEE_FLT_EXPONENT_BIAS) << IEEE_FLT_MANTISSA_BITS;
    float y = *reinterpret_cast<float *>(&exponent);
    x -= (float)i;
    if (x >= 0.5f) {
        x -= 0.5f;
        y *= 1.4142135623730950488f;	// multiply with sqrt( 2 )
    }
    float x2 = x * x;
    float p = x * (7.2152891511493f + x2 * 0.0576900723731f);
    float q = 20.8189237930062f + x2;
    x = y * (q + p) / (q - p);
    return x;
}

BE_INLINE double Math::Exp64(float f) {
    return exp(f);
}

BE_INLINE float Math::Log(float f) {
    return logf(f);
}

BE_INLINE float Math::Log16(float f) {
    int i = *reinterpret_cast<int *>(&f);
    int exponent = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
    i -= (exponent + 1 ) << IEEE_FLT_MANTISSA_BITS;	// get value in the range [.5, 1>
    float y = *reinterpret_cast<float *>(&i);
    y *= 1.4142135623730950488f;					// multiply with sqrt( 2 )
    y = (y - 1.0f) / (y + 1.0f);
    float y2 = y * y;
    y = y * (2.000000000046727f + y2 * (0.666666635059382f + y2 * (0.4000059794795f + y2 * (0.28525381498f + y2 * 0.2376245609f))));
    y += 0.693147180559945f * ((float)exponent + 0.5f);
    return y;
}

BE_INLINE double Math::Log64(float f) {
    return log(f);
}

BE_INLINE float Math::Log(float b, float f) {
    return Log(f) / Log(b);
}

BE_INLINE float Math::Log16(float b, float f) {
    return Log16(f) / Log16(b);
}

BE_INLINE double Math::Log64(float b, float f) {
    return Log64(f) / Log64(b);
}

BE_INLINE int Math::IPow(int x, int y) {
    int r; 
    for (r = x; y > 1; y--) {
        r *= x;
    }
    return r;
}

BE_INLINE int Math::ILog2(float f) {
    return (((*reinterpret_cast<int *>(&f)) >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
}

BE_INLINE int Math::ILog2(int i) {
    return ILog2((float)i);
}

BE_INLINE int Math::BitsForFloat(float f) {
    return ILog2(f) + 1;
}

BE_INLINE int Math::BitsForInteger(int i) {
    return ILog2((float)i) + 1;
}

BE_INLINE int Math::FloorPowerOfTwo(int x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x >>= 1;
    x++;
    return x;
}

BE_INLINE int Math::CeilPowerOfTwo(int x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

BE_INLINE bool Math::IsPowerOfTwo(int x) {
    return (x & (x - 1)) == 0 && x > 0;
}

BE_INLINE int Math::Abs(int x) {
   int y = x >> 31;
   return ((x ^ y) - y);
}

BE_INLINE float Math::Fabs(float f) { 
#if 0
    if (f >= 0) {
        return f;
    }
    return -f;
#else
    int tmp = *reinterpret_cast<int *>(&f);
    tmp &= 0x7FFFFFFF;
    return *reinterpret_cast<float *>(&tmp);
#endif
}

BE_INLINE float Math::Floor(float f) {
    return floorf(f);
}

BE_INLINE float Math::Ceil(float f) {
    return ceilf(f);
}

BE_INLINE float Math::Fract(float f) {
    return f - floorf(f);
}

BE_INLINE float Math::Rint(float f) {
    return floorf(f + 0.5f);
}

BE_INLINE int Math::Ftoi(float f) {
    return (int)f;
}

BE_INLINE int Math::FtoiFast(float f) {
#if BE_WIN_X86_SSE_INTRIN
    // If a converted result is larger than the maximum signed doubleword integer,
    // the floating-point invalid exception is raised, and if this exception is masked,
    // the indefinite integer value (80000000H) is returned.
    __m128 x = _mm_load_ss(&f);
    return _mm_cvttss_si32(x);
#elif 0 // round chop (C/C++ standard)
    int i = *reinterpret_cast<int *>(&f);
    int s = i >> IEEE_FLT_SIGN_BIT;
    int e = ((i >> IEEE_FLT_MANTISSA_BITS) & ((1 << IEEE_FLT_EXPONENT_BITS) - 1)) - IEEE_FLT_EXPONENT_BIAS;
    int m = (i & ((1 << IEEE_FLT_MANTISSA_BITS) - 1)) | (1 << IEEE_FLT_MANTISSA_BITS);
    int shift = e - IEEE_FLT_MANTISSA_BITS;
    return ((((m >> -shift) | (m << shift)) & ~(e >> 31)) ^ s) - s;
#else
    // If a converted result is larger than the maximum signed doubleword integer the result is undefined.
    return C_FLOAT_TO_INT(f);
#endif
}

BE_INLINE int8_t Math::Ftoi8(float f) {
#ifdef BE_WIN_X86_SSE_INTRIN
    __m128 x = _mm_load_ss(&f);
    x = _mm_max_ss(x, SIMD_SP_min_char);
    x = _mm_min_ss(x, SIMD_SP_max_char);
    return static_cast<int8_t>(_mm_cvttss_si32(x));
#else
    // The converted result is clamped to the range [-128, 127].
    int i = C_FLOAT_TO_INT(f);
    if (i < -128) {
        return -128;
    } else if (i > 127) {
        return 127;
    }
    return static_cast<int8_t>(i);
#endif
}

BE_INLINE int16_t Math::Ftoi16(float f) {
#ifdef BE_WIN_X86_SSE_INTRIN
    __m128 x = _mm_load_ss(&f);
    x = _mm_max_ss(x, SIMD_SP_min_short);
    x = _mm_min_ss(x, SIMD_SP_max_short);
    return static_cast<int16_t>(_mm_cvttss_si32(x));
#else
    // The converted result is clamped to the range [-32768, 32767].
    int i = C_FLOAT_TO_INT(f);
    if (i < -32768) {
        return -32768;
    } else if (i > 32767) {
        return 32767;
    }
    return static_cast<int16_t>(i);
#endif
}

BE_INLINE uint16_t Math::Ftoui16(float f) {
    // The converted result is clamped to the range [0, 65535].
    int i = C_FLOAT_TO_INT(f);
    if (i < 0) {
        return 0;
    } else if (i > 65535) {
        return 65535;
    }
    return static_cast<uint16_t>(i);
}

BE_INLINE byte Math::Ftob(float f) {
#ifdef BE_WIN_X86_SSE_INTRIN
    // If a converted result is negative the value (0) is returned and if the
    // converted result is larger than the maximum byte the value (255) is returned.
    __m128 x = _mm_load_ss(&f);
    x = _mm_max_ss(x, SIMD_SP_zero);
    x = _mm_min_ss(x, SIMD_SP_255);
    return static_cast<byte>(_mm_cvttss_si32(x));
#else
    // The converted result is clamped to the range [0,255].
    int i = C_FLOAT_TO_INT(f);
    if (i < 0) {
        return 0;
    } else if (i > 255) {
        return 255;
    }
    return static_cast<byte>(i);
#endif
}

BE_INLINE double Math::Factorial(unsigned int n) {
    const double table[16] = { 1., 1., 2., 6., 24., 120., 720., 5040., 40320., 362880., 3628800., 39916800., 479001600., 6227020800., 87178291200., 1307674368000. };
    double result = table[(n > 15) ? 15 : n];

    while (n >= 16) {
        result = result * double(n);
        n--;
    }

    return result;
}

BE_INLINE float Math::FloorSnap(float value, float snap) {
    return Math::Floor(value / snap) * snap;
}

BE_INLINE float Math::CeilSnap(float value, float snap) {
    return Math::Ceil(value / snap) * snap;
}

BE_INLINE float Math::AngleNormalize360(float angle) {
    if ((angle >= 360.0f) || (angle < 0.0f)) {
        angle -= floor(angle / 360.0f) * 360.0f;
    }
    return angle;
}

BE_INLINE float Math::AngleNormalize180(float angle) {
    angle = AngleNormalize360(angle);
    if (angle > 180.0f) {
        angle -= 360.0f;
    }

    return angle;
}

BE_INLINE float Math::AngleDelta(float angle1, float angle2) {
    return AngleNormalize180(angle1 - angle2);
}

BE_INLINE int Math::FloatHash(const float *arr, const int numFloats) {
    int i, hash = 0;
    const int *ptr;

    ptr = reinterpret_cast<const int *>(arr);
    for (i = 0; i < numFloats; i++) {
        hash ^= ptr[i];
    }

    return hash;
}

BE_INLINE float Math::Random(float minimum, float maximum) {
    //return minimum + (maximum - minimum) * ((::rand()%8193) / 8193.0f);
    return minimum + (maximum - minimum) * ((float)::rand() / (float)RAND_MAX);
}

template <typename T> 
BE_INLINE T	Lerp(const T p0, const T p1, float t) { 
    return p0 + ((p1 - p0) * t); 
}

template<> 
BE_INLINE int Lerp(const int p0, const int p1, float t) { 
    return Math::Ftoi((float)p0 + (((float)p1 - (float)p0) * t)); 
}

template <typename T>
BE_INLINE T	Cerp(const T p0, const T p1, const T p2, const T p3, float t) {
    T a = p3 - 3 * p2 + 3 * p1 - p0;
    T b = p2 - 2 * p1 + p0 - a; // simplified version of (-p3 + 4 * p2 - 5 * p1 + 2 * p0)
    T c = p2 - p0;
    T d = 2 * p1;
    return (t * (t * (t * a + b) + c) + d) * 0.5f;
}

// Fixed point linear interpolation
// f(x) = ax + b
// p0: f(0)
// p1: f(1)
// t: interpolater in range [0, 255]
BE_INLINE int FixedLerp(int p0, int p1, int t) {
    // p0 : f(0) = b
    // p1 : f(1) = a + b
    //
    // a = p1 - p0
    // b = p0
    return ((p0 << 8) + (p1 - p0) * t) >> 8;
}

// Fixed point cubic interpolation
// f(x) = ax^3 + bx^2 + cx + d
// p0: f(-1)
// p1: f(0)
// p2: f(1)
// p3: f(2)
// t: interpolater in range [0, 127]
BE_INLINE int FixedCerp(int p0, int p1, int p2, int p3, int t) {
#if 0
    // Hermite cubic spline with 4 points
    // f'(x) = 3ax^2 + 2bx + c
    //
    // p1 : f(0) = d
    // p2 : f(1) = a + b + c + d 
    // p2 - p0 : f'(0) = c
    // p3 - p1 : f'(1) = 3a + 2b + c
    //
    // |  0  0  0  1 | | a |   | p1      |
    // |  1  1  1  1 | | b | = | p2      |
    // |  0  0  1  0 | | c |   | p2 - p0 |
    // |  3  2  1  0 | | d |   | p3 - p1 |
    //
    // |  2 -2  1  1 | | p1      |   | a | 
    // | -3  3 -2 -1 | | p2      | = | b |
    // |  0  0  1  0 | | p2 - p0 |   | c |
    // |  1  0  0  0 | | p3 - p1 |   | d |
    //
    // a = 2*p1 - 2*p2 + (p2 - p0) + (p3 - p1) = p3 - p2 + p1 - p0
    // b = -3*p1 + 3*p2 - 2*(p2 - p0) - (p3 - p1) = -p3 + p2 - 2*p1 + 2*p0
    // c = p2 - p0
    // d = p1
    int p01 = p0 - p1;
    int a = (p3 - p2) - p01;
    int b = p01 - a;
    int c = p2 - p0;
    int d = p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 21;
#else
    // Catmull-Rom cubic spline with 4 points
    // f'(x) = 3ax^2 + 2bx + c
    //
    // p1 : f(0) = d
    // p2 : f(1) = a + b + c + d
    // (p2 - p0)/2 : f'(0) = c
    // (p3 - p1)/2 : f'(1) = 3a + 2b + c
    //
    // |  0  0  0  1 | | a |   | p1      |
    // |  1  1  1  1 | | b | = | p2      |
    // |  0  0  2  0 | | c |   | p2 - p0 |
    // |  6  4  2  0 | | d |   | p3 - p1 |
    //
    //    |  4 -4  1  1 | | p1      |   | a | 
    // 1/2| -6  6 -2 -1 | | p2      | = | b |
    //    |  0  0  1  0 | | p2 - p0 |   | c |
    //    |  2  0  0  0 | | p3 - p1 |   | d |
    //
    // a = 4*p1 - 4*p2 + (p2 - p0) + (p3 - p1) = (p3 - 3*p2 + 3*p1 - p0) / 2
    // b = -6*p1 + 6*p2 - 2*(p2 - p0) - (p3 - p1) = (-p3 + 4*p2 - 5p1 + 2*p0) / 2
    // c = (p2 - p0) / 2
    // d = p1
    int a = p3 - 3 * p2 + 3 * p1 - p0;
    int b = p2 - 2 * p1 + p0 - a;
    int c = p2 - p0;
    int d = 2 * p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 22;
#endif
}

BE_NAMESPACE_END
