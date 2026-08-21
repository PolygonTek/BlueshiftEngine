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

#include "BlueshiftEngine.h"
#include "Test.h"

#include <cstdio>
#include <cstdlib>

static constexpr int TestCount = 4096;

static void GetBest(uint64_t start, uint64_t end, uint64_t &best) {
    uint64_t elapsed = end - start;
    if (!best || elapsed < best) {
        best = elapsed;
    }
}

static void PrintClocksGeneric(const char *string, uint64_t clocks) {
    BE_LOG("Generic->%s: %" PRIu64 " clocks\n", string, clocks);
}

static void PrintClocksSIMD(const char *string, uint64_t clocksGeneric, uint64_t clocksSIMD) {
    BE_LOG("   SIMD->%s: %" PRIu64 " clocks (%.2fx fast)\n", string, clocksSIMD, (float)clocksGeneric / (float)clocksSIMD);
}

static void RandomFloatArrayInit(float *dst, int count, float minimum, float maximum) {
    for (int i = 0; i < count; i++) {
        dst[i] = BE1::RNG::GetRandom(minimum, maximum);
    }
}

static void VerifySIMD(bool expression, int line) {
    if (!expression) {
        std::fprintf(stderr, "SIMD verification failed at line %d\n", line);
        BE_FATALERROR("SIMD verification failed at line %d\n", line);
        std::abort();
    }
}

#define VERIFY_SIMD_PRIMITIVE(expression) VerifySIMD((expression), __LINE__)

#if defined(HAVE_X86_SSE_INTRIN)
static void TestSSEPrimitives() {
    ALIGN_AS16 int32_t intOutput[4];
    const int32_t valueA[4] = { 0x7fffffff, -3, 0x40000000, -1 };
    const int32_t valueB[4] = { 2, -7, 4, INT32_MIN };
    store_si128(set_epi32(valueA[0], valueA[1], valueA[2], valueA[3]) * set_epi32(valueB[0], valueB[1], valueB[2], valueB[3]), intOutput);
    for (int i = 0; i < 4; i++) {
        VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[i] == (uint32_t)valueA[i] * (uint32_t)valueB[i]);
    }

    const int32_t shiftInput[4] = { -8, -1, 0x40000000, INT32_MIN };
    store_si128(set_epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3]) << 1, intOutput);
    for (int i = 0; i < 4; i++) {
        VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[i] == ((uint32_t)shiftInput[i] << 1));
    }
    store_si128(set_epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3]) >> 2, intOutput);
    const int32_t expectedRightShift[4] = { -2, -1, 0x10000000, INT32_MIN / 4 };
    for (int i = 0; i < 4; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == expectedRightShift[i]);
    }

    ALIGN_AS16 float floatOutput[4];
    const ssef roundInput = set_ps(-2.5f, -1.5f, 1.5f, 2.5f);
    store_ps(round_ps(roundInput), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == -2.0f && floatOutput[1] == -2.0f && floatOutput[2] == 2.0f && floatOutput[3] == 2.0f);

    const ssef directionInput = set_ps(-1.0f, -1.25f, 1.0f, 1.25f);
    store_ps(floor_ps(directionInput), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == -1.0f && floatOutput[1] == -2.0f && floatOutput[2] == 1.0f && floatOutput[3] == 1.0f);
    store_ps(ceil_ps(directionInput), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == -1.0f && floatOutput[1] == -1.0f && floatOutput[2] == 1.0f && floatOutput[3] == 2.0f);
    store_ps(trunc_ps(directionInput), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == -1.0f && floatOutput[1] == -1.0f && floatOutput[2] == 1.0f && floatOutput[3] == 1.0f);

    const sseb unpacked = unpackhi_b32(ps_to_b32(set_ps(1.0f, 2.0f, 3.0f, 4.0f)), ps_to_b32(set_ps(5.0f, 6.0f, 7.0f, 8.0f)));
    store_ps(unpacked, floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == 3.0f && floatOutput[1] == 7.0f && floatOutput[2] == 4.0f && floatOutput[3] == 8.0f);

    const ssei shifted = bx_mm_sld_si128(set_epi32(0, 1, 2, 3), set_epi32(4, 5, 6, 7), 8);
    store_si128(shifted, intOutput);
    VERIFY_SIMD_PRIMITIVE(intOutput[0] == 2 && intOutput[1] == 3 && intOutput[2] == 4 && intOutput[3] == 5);

    ssei bytes = setzero_si128();
    for (int i = 0; i < 16; i++) {
        bytes.i8[i] = (int8_t)i;
    }
    VERIFY_SIMD_PRIMITIVE(extract_epi8<15>(bytes) == 15);

    const ssef viewSource = set_ps(1.0f, -0.0f, 0.5f, -2.0f);
    const ssei &integerView = viewSource;
    store_si128(integerView, intOutput);
    VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[0] == 0x3f800000u && (uint32_t)intOutput[1] == 0x80000000u && (uint32_t)intOutput[2] == 0x3f000000u && (uint32_t)intOutput[3] == 0xc0000000u);

    const sseb &booleanView = viewSource;
    store_ps(booleanView, floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == 1.0f && floatOutput[1] == 0.0f && floatOutput[2] == 0.5f && floatOutput[3] == -2.0f);

    const ssei blendMask = set_epi32(1, INT32_MIN, INT32_MAX, -1);
    store_ps(_mm_blendv_ps(set_ps(1.0f, 2.0f, 3.0f, 4.0f), set_ps(10.0f, 20.0f, 30.0f, 40.0f), _mm_castsi128_ps(blendMask)), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == 1.0f && floatOutput[1] == 20.0f && floatOutput[2] == 3.0f && floatOutput[3] == 40.0f);

    ssei byteValue = setzero_si128();
    ssei byteInput = setzero_si128();
    ssei byteMask = setzero_si128();
    for (int i = 0; i < 16; i++) {
        byteValue.i8[i] = (int8_t)i;
        byteInput.i8[i] = (int8_t)(i + 32);
        byteMask.i8[i] = (i & 1) ? (int8_t)0x80 : (int8_t)0x7f;
    }
    const ssei byteBlend = _mm_blendv_epi8(byteValue, byteInput, byteMask);
    for (int i = 0; i < 16; i++) {
        VERIFY_SIMD_PRIMITIVE(byteBlend.i8[i] == ((i & 1) ? byteInput.i8[i] : byteValue.i8[i]));
    }
}
#endif

#if defined(HAVE_X86_AVX_INTRIN)
static void TestAVXPrimitives() {
    ALIGN_AS32 int32_t intOutput[8];
    const int32_t shiftInput[8] = { -8, -4, -1, 0, 4, 8, INT32_MIN, INT32_MAX };
    store_si256(set_256epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3], shiftInput[4], shiftInput[5], shiftInput[6], shiftInput[7]) << 1, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[i] == ((uint32_t)shiftInput[i] << 1));
    }

    store_si256(set_256epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3], shiftInput[4], shiftInput[5], shiftInput[6], shiftInput[7]) >> 2, intOutput);
    const int32_t expectedShift[8] = { -2, -1, -1, 0, 1, 2, INT32_MIN / 4, INT32_MAX / 4 };
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == expectedShift[i]);
    }

    const int32_t inputA[8] = { -100, -10, -1, 0, 1, 10, 100, 1000 };
    const int32_t inputB[8] = { 3, -20, -1, 4, 2, 5, 200, -1000 };
    const avxi intA = set_256epi32(inputA[0], inputA[1], inputA[2], inputA[3], inputA[4], inputA[5], inputA[6], inputA[7]);
    const avxi intB = set_256epi32(inputB[0], inputB[1], inputB[2], inputB[3], inputB[4], inputB[5], inputB[6], inputB[7]);

    store_si256(intA + intB, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == inputA[i] + inputB[i]);
    }
    store_si256(intA - intB, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == inputA[i] - inputB[i]);
    }
    store_si256(intA * intB, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == inputA[i] * inputB[i]);
    }
    store_si256(min_256epi32(intA, intB), intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == (inputA[i] < inputB[i] ? inputA[i] : inputB[i]));
    }
    store_si256(max_256epi32(intA, intB), intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == (inputA[i] > inputB[i] ? inputA[i] : inputB[i]));
    }
    store_si256(intA == intB, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == (inputA[i] == inputB[i] ? -1 : 0));
    }
    store_si256(intA > intB, intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == (inputA[i] > inputB[i] ? -1 : 0));
    }
    store_si256(srl_256epi32(intA, 3), intOutput);
    for (int i = 0; i < 8; i++) {
        VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[i] == ((uint32_t)inputA[i] >> 3));
    }

    const avxf value = _mm256_castsi256_ps(set_256epi32(0x7fc00000, 0, 0, 0, 0, 0, 0, 0));
    const avxf zero = setzero_256ps();
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value == zero) & 1) == 0);
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value != zero) & 1) != 0);
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value < zero) & 1) == 0);
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value > zero) & 1) == 0);
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value >= zero) & 1) == 0);
    VERIFY_SIMD_PRIMITIVE((_mm256_movemask_ps(value <= zero) & 1) == 0);
}
#endif

#if defined(HAVE_ARM_NEON_INTRIN)
static void TestNEONPrimitives() {
    ALIGN_AS16 int32_t intOutput[4];
    const int32_t shiftInput[4] = { -8, -1, 0x40000000, INT32_MIN };
    store_si128(set_epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3]) << 1, intOutput);
    for (int i = 0; i < 4; i++) {
        VERIFY_SIMD_PRIMITIVE((uint32_t)intOutput[i] == ((uint32_t)shiftInput[i] << 1));
    }
    store_si128(set_epi32(shiftInput[0], shiftInput[1], shiftInput[2], shiftInput[3]) >> 2, intOutput);
    const int32_t expectedRightShift[4] = { -2, -1, 0x10000000, INT32_MIN / 4 };
    for (int i = 0; i < 4; i++) {
        VERIFY_SIMD_PRIMITIVE(intOutput[i] == expectedRightShift[i]);
    }

    ALIGN_AS16 float floatOutput[4];
    store_ps(shuffle_ps<2, 3, 2, 3>(set_ps(0.0f, 1.0f, 2.0f, 3.0f), set_ps(4.0f, 5.0f, 6.0f, 7.0f)), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == 2.0f && floatOutput[1] == 3.0f && floatOutput[2] == 6.0f && floatOutput[3] == 7.0f);

    ALIGN_AS16 uint32_t bitOutput[4];
    vst1q_u32(bitOutput, ps_to_b32(set_ps(1.0f, -0.0f, 0.5f, -2.0f)));
    VERIFY_SIMD_PRIMITIVE(bitOutput[0] == 0x3f800000 && bitOutput[1] == 0x80000000 && bitOutput[2] == 0x3f000000 && bitOutput[3] == 0xc0000000);

    store_ps(round_ps(set_ps(-2.5f, -1.5f, 1.5f, 2.5f)), floatOutput);
    VERIFY_SIMD_PRIMITIVE(floatOutput[0] == -2.0f && floatOutput[1] == -2.0f && floatOutput[2] == 2.0f && floatOutput[3] == 2.0f);

    store_si128(ps_to_epi32(set_ps(-2.5f, -1.5f, 1.5f, 2.5f)), intOutput);
    VERIFY_SIMD_PRIMITIVE(intOutput[0] == -2 && intOutput[1] == -2 && intOutput[2] == 2 && intOutput[3] == 2);

    store_si128(abs_epi32(set_epi32(INT32_MIN, -7, 0, 9)), intOutput);
    VERIFY_SIMD_PRIMITIVE(intOutput[0] == INT32_MIN && intOutput[1] == 7 && intOutput[2] == 0 && intOutput[3] == 9);

    neoni shuffleInput = setzero_si128();
    neoni shuffleIndices = setzero_si128();
    for (int i = 0; i < 16; i++) {
        shuffleInput.i8[i] = (int8_t)(0x80 + i);
        shuffleIndices.i8[i] = (int8_t)(15 - i);
    }
    const neoni shuffledBytes = bx_vshufq_s8(shuffleInput, shuffleIndices);
    for (int i = 0; i < 16; i++) {
        VERIFY_SIMD_PRIMITIVE((uint8_t)shuffledBytes.i8[i] == (uint8_t)(0x8f - i));
    }
}
#endif


static void VerifyFloatArrayResults(const char *operation, const float *expected, const float *actual, const int count) {
    for (int i = 0; i < count; i++) {
        const float tolerance = 1e-5f * (1.0f + BE1::Math::Abs(expected[i]));
        const bool equal = expected[i] == actual[i] || BE1::Math::Abs(expected[i] - actual[i]) <= tolerance;
        if (!equal) {
            std::fprintf(stderr, "SIMD %s verification failed at index %d: expected %g, actual %g\n", operation, i, expected[i], actual[i]);
            BE_FATALERROR("SIMD %s verification failed at index %d: expected %g, actual %g\n", operation, i, expected[i], actual[i]);
            std::abort();
        }
    }
}

static void VerifyFloatResult(const char *operation, const float expected, const float actual) {
    const float tolerance = 1e-5f * (1.0f + BE1::Math::Abs(expected));
    const bool equal = expected == actual || BE1::Math::Abs(expected - actual) <= tolerance;
    if (!equal) {
        std::fprintf(stderr, "SIMD %s verification failed: expected %g, actual %g\n", operation, expected, actual);
        BE_FATALERROR("SIMD %s verification failed: expected %g, actual %g\n", operation, expected, actual);
        std::abort();
    }
}

static void TestAdd() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    ALIGN_AS32 float expected[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    BE1::simdGeneric->Add(expected, c, src0, COUNT_OF(dst));
    BE1::simdProcessor->Add(dst, c, src0, COUNT_OF(dst));
    VerifyFloatArrayResults("Add constant 8192", expected, dst, COUNT_OF(dst));

    BE1::simdGeneric->Add(expected, src0, src1, COUNT_OF(dst));
    BE1::simdProcessor->Add(dst, src0, src1, COUNT_OF(dst));
    VerifyFloatArrayResults("Add arrays 8192", expected, dst, COUNT_OF(dst));

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Add( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Add( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Add( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Add( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
}

static void TestSub() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    ALIGN_AS32 float expected[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    BE1::simdGeneric->Sub(expected, c, src0, COUNT_OF(dst));
    BE1::simdProcessor->Sub(dst, c, src0, COUNT_OF(dst));
    VerifyFloatArrayResults("Sub constant 8192", expected, dst, COUNT_OF(dst));

    BE1::simdGeneric->Sub(expected, src0, src1, COUNT_OF(dst));
    BE1::simdProcessor->Sub(dst, src0, src1, COUNT_OF(dst));
    VerifyFloatArrayResults("Sub arrays 8192", expected, dst, COUNT_OF(dst));

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Sub( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Sub( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Sub( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Sub( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
}

static void TestMul() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    ALIGN_AS32 float expected[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    BE1::simdGeneric->Mul(expected, c, src0, COUNT_OF(dst));
    BE1::simdProcessor->Mul(dst, c, src0, COUNT_OF(dst));
    VerifyFloatArrayResults("Mul constant 8192", expected, dst, COUNT_OF(dst));

    BE1::simdGeneric->Mul(expected, src0, src1, COUNT_OF(dst));
    BE1::simdProcessor->Mul(dst, src0, src1, COUNT_OF(dst));
    VerifyFloatArrayResults("Mul arrays 8192", expected, dst, COUNT_OF(dst));

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Mul( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Mul( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Mul( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Mul( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
}

static void TestDiv() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    ALIGN_AS32 float expected[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), 1.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), 1.0f, 100.0f);

    BE1::simdGeneric->Div(expected, c, src0, COUNT_OF(dst));
    BE1::simdProcessor->Div(dst, c, src0, COUNT_OF(dst));
    VerifyFloatArrayResults("Div constant 8192", expected, dst, COUNT_OF(dst));

    BE1::simdGeneric->Div(expected, src0, src1, COUNT_OF(dst));
    BE1::simdProcessor->Div(dst, src0, src1, COUNT_OF(dst));
    VerifyFloatArrayResults("Div arrays 8192", expected, dst, COUNT_OF(dst));

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Div( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Div( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Div( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Div( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
}

static void TestSum() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src[8192];
    float sum;
    const int boundaryCounts[] = { 0, 1, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 48, 64 };
    ALIGN_AS32 float boundarySrc[64];
    for (int i = 0; i < COUNT_OF(boundarySrc); i++) {
        boundarySrc[i] = (float)(i + 1);
    }

    for (int i = 0; i < COUNT_OF(boundaryCounts); i++) {
        const int count = boundaryCounts[i];
        const float expected = BE1::simdGeneric->Sum(boundarySrc, count);
        const float actual = BE1::simdProcessor->Sum(boundarySrc, count);
        if (expected != actual) {
            std::fprintf(stderr, "SIMD Sum boundary verification failed for count %d: expected %g, actual %g\n", count, expected, actual);
            BE_FATALERROR("SIMD Sum boundary verification failed for count %d: expected %g, actual %g\n", count, expected, actual);
            std::abort();
        }
    }

    RandomFloatArrayInit(src, COUNT_OF(src), 0.0f, 100.0f);
    const float expectedSum = BE1::simdGeneric->Sum(src, COUNT_OF(src));
    const float actualSum = BE1::simdProcessor->Sum(src, COUNT_OF(src));
    VerifyFloatResult("Sum 8192", expectedSum, actualSum);

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        sum = BE1::simdGeneric->Sum(src, COUNT_OF(src));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Sum 8192", bestClocksGeneric);

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        sum = BE1::simdProcessor->Sum(src, COUNT_OF(src));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Sum 8192", bestClocksGeneric, bestClocksSIMD);
}

static void TestTransformPoints() {
    static const int numVectors = 1024;
    static const int benchmarkIterations = 64;
    const int boundaryCounts[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17 };
    const BE1::Mat3x4 matrix(
        0.75f, -0.25f, 0.50f, 13.0f,
        0.40f, 1.25f, -0.30f, -7.0f,
        -0.60f, 0.20f, 0.90f, 3.5f);
    ALIGN_AS32 BE1::Vec3 source[numVectors];
    ALIGN_AS32 BE1::Vec3 expected[numVectors];
    ALIGN_AS32 BE1::Vec3 actual[numVectors];
    ALIGN_AS32 BE1::Vec3 member[numVectors];

    RandomFloatArrayInit((float *)source, numVectors * 3, -100.0f, 100.0f);

    for (int i = 0; i < COUNT_OF(boundaryCounts); i++) {
        const int count = boundaryCounts[i];
        memcpy(expected, source, sizeof(expected));
        memcpy(actual, source, sizeof(actual));
        memcpy(member, source, sizeof(member));

        BE1::simdGeneric->TransformPoints(expected, matrix, count);
        BE1::simdProcessor->TransformPoints(actual, matrix, count);
        for (int j = 0; j < count; j++) {
            member[j] = matrix.TransformPos(member[j]);
        }
        VerifyFloatArrayResults("TransformPoints boundary", (const float *)expected, (const float *)actual, count * 3);
        VerifyFloatArrayResults("TransformPos loop boundary", (const float *)expected, (const float *)member, count * 3);
    }

    uint64_t bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(expected, source, sizeof(expected));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            BE1::simdGeneric->TransformPoints(expected, matrix, numVectors);
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("TransformPoints 1024 x 64", bestClocksGeneric);

    uint64_t bestClocksMember = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(member, source, sizeof(member));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            for (int k = 0; k < numVectors; k++) {
                member[k] = matrix.TransformPos(member[k]);
            }
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksMember);
    }

    BE_LOG(" Member->TransformPos loop 1024 x 64: %" PRIu64 " clocks\n", bestClocksMember);

    uint64_t bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(actual, source, sizeof(actual));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            BE1::simdProcessor->TransformPoints(actual, matrix, numVectors);
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("TransformPoints 1024 x 64", bestClocksGeneric, bestClocksSIMD);
    BE_LOG("   SIMD batch vs member loop: %.2fx fast\n", (float)bestClocksMember / (float)bestClocksSIMD);
}

static void TestTransformDirections() {
    static const int numVectors = 1024;
    static const int benchmarkIterations = 64;
    const int boundaryCounts[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17 };
    const BE1::Mat3x4 matrix(
        0.75f, -0.25f, 0.50f, 13.0f,
        0.40f, 1.25f, -0.30f, -7.0f,
        -0.60f, 0.20f, 0.90f, 3.5f);
    ALIGN_AS32 BE1::Vec3 source[numVectors];
    ALIGN_AS32 BE1::Vec3 expected[numVectors];
    ALIGN_AS32 BE1::Vec3 actual[numVectors];
    ALIGN_AS32 BE1::Vec3 member[numVectors];

    RandomFloatArrayInit((float *)source, numVectors * 3, -100.0f, 100.0f);

    for (int i = 0; i < COUNT_OF(boundaryCounts); i++) {
        const int count = boundaryCounts[i];
        memcpy(expected, source, sizeof(expected));
        memcpy(actual, source, sizeof(actual));
        memcpy(member, source, sizeof(member));

        BE1::simdGeneric->TransformDirections(expected, matrix, count);
        BE1::simdProcessor->TransformDirections(actual, matrix, count);
        for (int j = 0; j < count; j++) {
            member[j] = matrix.TransformDir(member[j]);
        }
        VerifyFloatArrayResults("TransformDirections boundary", (const float *)expected, (const float *)actual, count * 3);
        VerifyFloatArrayResults("TransformDir loop boundary", (const float *)expected, (const float *)member, count * 3);
    }

    uint64_t bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(expected, source, sizeof(expected));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            BE1::simdGeneric->TransformDirections(expected, matrix, numVectors);
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("TransformDirections 1024 x 64", bestClocksGeneric);

    uint64_t bestClocksMember = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(member, source, sizeof(member));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            for (int k = 0; k < numVectors; k++) {
                member[k] = matrix.TransformDir(member[k]);
            }
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksMember);
    }

    BE_LOG(" Member->TransformDir loop 1024 x 64: %" PRIu64 " clocks\n", bestClocksMember);

    uint64_t bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        memcpy(actual, source, sizeof(actual));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < benchmarkIterations; j++) {
            BE1::simdProcessor->TransformDirections(actual, matrix, numVectors);
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("TransformDirections 1024 x 64", bestClocksGeneric, bestClocksSIMD);
    BE_LOG("   SIMD batch vs member loop: %.2fx fast\n", (float)bestClocksMember / (float)bestClocksSIMD);
}

static void ClearCache() {
    size_t size = 1024 * 1024 * 512; // 512MB
    char *buffer = (char *)malloc(size);
    if (buffer) {
        for (size_t i = 0; i < size; i += 4096) {
            buffer[i] = 0; // 페이지를 메모리에 로드
        }
        free(buffer);
    }
}

static void TestMemcpy() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 1024 * 256 + 255;

    ClearCache();

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        unsigned char *bufferSrc = (unsigned char *)Mem_Alloc16(bufferSize);
        unsigned char *bufferDst = (unsigned char *)Mem_Alloc16(bufferSize);

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);

        Mem_AlignedFree(bufferSrc);
        Mem_AlignedFree(bufferDst);
    }

    PrintClocksGeneric(BE1::va("Memcpy %i Bytes", bufferSize), bestClocksGeneric);

    ClearCache();

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        unsigned char *bufferSrc = (unsigned char *)Mem_Alloc16(bufferSize);
        unsigned char *bufferDst = (unsigned char *)Mem_Alloc16(bufferSize);

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);

        Mem_AlignedFree(bufferSrc);
        Mem_AlignedFree(bufferDst);
    }

    PrintClocksSIMD(BE1::va("Memcpy %i Bytes", bufferSize), bestClocksGeneric, bestClocksSIMD);
}

static void TestMemset() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 1024 * 128 + 255;

    ClearCache();

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        unsigned char *buffer = (unsigned char *)Mem_Alloc16(bufferSize * 64);

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Memset(buffer, 170, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);

        Mem_AlignedFree(buffer);
    }

    PrintClocksGeneric(BE1::va("Memset %i Bytes", bufferSize), bestClocksGeneric);

    ClearCache();

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        unsigned char *buffer = (unsigned char *)Mem_Alloc16(bufferSize * 64);

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Memset(buffer, 170, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);

        Mem_AlignedFree(buffer);
    }

    PrintClocksSIMD(BE1::va("Memset %i Bytes", bufferSize), bestClocksGeneric, bestClocksSIMD);
}

#if defined(HAVE_X86_AVX_INTRIN)
static void TestTranspose8x8() {
    ALIGN_AS32 float src[8 * 8];
    ALIGN_AS32 float dst[8 * 8];

    for (int i = 0; i < COUNT_OF(src); i++) {
        src[i] = (float)i;
    }

    simd8f c0, c1, c2, c3, c4, c5, c6, c7;
    BE1::transpose8x8(load_256ps(src + 0 * 8), load_256ps(src + 1 * 8),
        load_256ps(src + 2 * 8), load_256ps(src + 3 * 8),
        load_256ps(src + 4 * 8), load_256ps(src + 5 * 8),
        load_256ps(src + 6 * 8), load_256ps(src + 7 * 8),
        c0, c1, c2, c3, c4, c5, c6, c7);

    store_256ps(c0, dst + 0 * 8);
    store_256ps(c1, dst + 1 * 8);
    store_256ps(c2, dst + 2 * 8);
    store_256ps(c3, dst + 3 * 8);
    store_256ps(c4, dst + 4 * 8);
    store_256ps(c5, dst + 5 * 8);
    store_256ps(c6, dst + 6 * 8);
    store_256ps(c7, dst + 7 * 8);

    for (int row = 0; row < 8; row++) {
        for (int column = 0; column < 8; column++) {
            VERIFY_SIMD_PRIMITIVE(dst[row * 8 + column] == src[column * 8 + row]);
        }
    }
}
#endif

static void RandomJointPoseArrayInit(BE1::JointPose *joints, int numJoints, bool alternateQuaternionSign) {
    RandomFloatArrayInit((float *)joints, numJoints * (int)(sizeof(BE1::JointPose) / sizeof(float)), -1.0f, 1.0f);

    for (int i = 0; i < numJoints; i++) {
        joints[i].q.Set(
            BE1::RNG::GetRandom(-1.0f, 1.0f),
            BE1::RNG::GetRandom(-1.0f, 1.0f),
            BE1::RNG::GetRandom(-1.0f, 1.0f),
            1.0f);
        joints[i].q.Normalize();
        if (alternateQuaternionSign && (i & 1)) {
            joints[i].q = -joints[i].q;
        }

        joints[i].t.Set(
            BE1::RNG::GetRandom(-10.0f, 10.0f),
            BE1::RNG::GetRandom(-10.0f, 10.0f),
            BE1::RNG::GetRandom(-10.0f, 10.0f));
        joints[i].s.Set(
            BE1::RNG::GetRandom(0.5f, 1.5f),
            BE1::RNG::GetRandom(0.5f, 1.5f),
            BE1::RNG::GetRandom(0.5f, 1.5f));
    }
}

static void VerifyJointPoseArray(const BE1::JointPose *expected, const BE1::JointPose *actual, int numJoints, float quatEpsilon, float vectorEpsilon) {
    for (int i = 0; i < numJoints; i++) {
        VERIFY_SIMD_PRIMITIVE(actual[i].Equals(expected[i], quatEpsilon, vectorEpsilon, vectorEpsilon));
    }
}

static void TestTransformJointsCorrectness() {
    ALIGN_AS32 BE1::Mat3x4 localJointMatrix[4];
    ALIGN_AS32 BE1::Mat3x4 expectedJointMatrix[4];
    ALIGN_AS32 BE1::Mat3x4 actualJointMatrix[4];
    const BE1::Vec3 translations[4] = {
        BE1::Vec3(1.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 2.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 3.0f),
        BE1::Vec3(4.0f, 0.0f, 0.0f)
    };

    for (int i = 0; i < COUNT_OF(localJointMatrix); i++) {
        localJointMatrix[i] = BE1::Mat3x4::identity;
        localJointMatrix[i].SetTranslation(translations[i]);
        expectedJointMatrix[i] = localJointMatrix[i];
        actualJointMatrix[i] = localJointMatrix[i];
    }

    const int multipleRootParents[4] = { -1, 0, -1, 2 };
    BE1::simdGeneric->TransformJoints(expectedJointMatrix, multipleRootParents, 0, 3);
    BE1::simdProcessor->TransformJoints(actualJointMatrix, multipleRootParents, 0, 3);

    for (int i = 0; i < COUNT_OF(expectedJointMatrix); i++) {
        VERIFY_SIMD_PRIMITIVE(actualJointMatrix[i].Equals(expectedJointMatrix[i], 0.0001f));
        expectedJointMatrix[i] = localJointMatrix[i];
        actualJointMatrix[i] = localJointMatrix[i];
    }

    const int partialRangeParents[4] = { -1, 0, 1, 2 };
    BE1::simdGeneric->TransformJoints(expectedJointMatrix, partialRangeParents, 0, 3);
    BE1::simdProcessor->TransformJoints(actualJointMatrix, partialRangeParents, 0, 1);
    BE1::simdProcessor->TransformJoints(actualJointMatrix, partialRangeParents, 2, 3);

    for (int i = 0; i < COUNT_OF(expectedJointMatrix); i++) {
        VERIFY_SIMD_PRIMITIVE(actualJointMatrix[i].Equals(expectedJointMatrix[i], 0.0001f));
    }
}

static void TestUntransformJointsCorrectness() {
    ALIGN_AS32 BE1::Mat3x4 localJointMatrix[4] = {
        BE1::Mat3x4(0, -1, 0, 1, 1, 0, 0, 2, 0, 0, 1, 3),
        BE1::Mat3x4(1, 0, 0, 4, 0, 0, -1, 5, 0, 1, 0, 6),
        BE1::Mat3x4(0, 0, 1, 7, 0, 1, 0, 8, -1, 0, 0, 9),
        BE1::Mat3x4(0, 1, 0, -1, -1, 0, 0, -2, 0, 0, 1, -3)
    };
    ALIGN_AS32 BE1::Mat3x4 expectedJointMatrix[4];
    ALIGN_AS32 BE1::Mat3x4 actualJointMatrix[4];
    const int parents[4] = { -1, 0, 1, 1 };

    for (int i = 0; i < COUNT_OF(localJointMatrix); i++) {
        expectedJointMatrix[i] = localJointMatrix[i];
    }

    BE1::simdGeneric->TransformJoints(expectedJointMatrix, parents, 0, COUNT_OF(expectedJointMatrix) - 1);

    for (int i = 0; i < COUNT_OF(actualJointMatrix); i++) {
        actualJointMatrix[i] = expectedJointMatrix[i];
    }

    BE1::simdGeneric->UntransformJoints(expectedJointMatrix, parents, 0, COUNT_OF(expectedJointMatrix) - 1);
#if defined(HAVE_X86_AVX_INTRIN)
    BE1::SIMD_8 simd8;
    simd8.UntransformJoints(actualJointMatrix, parents, 0, COUNT_OF(actualJointMatrix) - 1);
#else
    BE1::simdProcessor->UntransformJoints(actualJointMatrix, parents, 0, COUNT_OF(actualJointMatrix) - 1);
#endif

    for (int i = 0; i < COUNT_OF(expectedJointMatrix); i++) {
        VERIFY_SIMD_PRIMITIVE(expectedJointMatrix[i].Equals(localJointMatrix[i], 0.0001f));
        VERIFY_SIMD_PRIMITIVE(actualJointMatrix[i].Equals(expectedJointMatrix[i], 0.0001f));
    }
}

#if defined(HAVE_X86_SSE_INTRIN) || defined(HAVE_ARM_NEON_INTRIN)
static void TestSIMD4JointOpsCorrectness() {
    static const int numJoints = 20;
    const int jointIndex[] = { 17, 3, 14, 0, 11, 6, 19, 2, 16, 8, 5, 13, 1 };
    ALIGN_AS32 BE1::JointPose sourceJoints[numJoints];
    ALIGN_AS32 BE1::JointPose blendJoints[numJoints];
    ALIGN_AS32 BE1::JointPose expectedJoints[numJoints];
    ALIGN_AS32 BE1::JointPose actualJoints[numJoints];
    BE1::CompressedJointPose compressedJoints[numJoints];
    BE1::SIMD_4 simd4;

    RandomJointPoseArrayInit(sourceJoints, numJoints, false);
    RandomJointPoseArrayInit(blendJoints, numJoints, true);

    for (int i = 0; i < numJoints; i++) {
        compressedJoints[i].q[0] = BE1::CompressedJointPose::QuatToShort(sourceJoints[i].q.x);
        compressedJoints[i].q[1] = BE1::CompressedJointPose::QuatToShort(sourceJoints[i].q.y);
        compressedJoints[i].q[2] = BE1::CompressedJointPose::QuatToShort(sourceJoints[i].q.z);
        compressedJoints[i].t[0] = BE1::CompressedJointPose::TranslationToShort(BE1::RNG::GetRandom(-3.0f, 3.0f));
        compressedJoints[i].t[1] = BE1::CompressedJointPose::TranslationToShort(BE1::RNG::GetRandom(-3.0f, 3.0f));
        compressedJoints[i].t[2] = BE1::CompressedJointPose::TranslationToShort(BE1::RNG::GetRandom(-3.0f, 3.0f));
        compressedJoints[i].s[0] = BE1::CompressedJointPose::ScaleToShort(sourceJoints[i].s.x);
        compressedJoints[i].s[1] = BE1::CompressedJointPose::ScaleToShort(sourceJoints[i].s.y);
        compressedJoints[i].s[2] = BE1::CompressedJointPose::ScaleToShort(sourceJoints[i].s.z);
    }

    memcpy(expectedJoints, sourceJoints, sizeof(expectedJoints));
    memcpy(actualJoints, sourceJoints, sizeof(actualJoints));
    BE1::simdGeneric->DecompressJoints(expectedJoints, compressedJoints, jointIndex, COUNT_OF(jointIndex));
    simd4.DecompressJoints(actualJoints, compressedJoints, jointIndex, COUNT_OF(jointIndex));
    VerifyJointPoseArray(expectedJoints, actualJoints, numJoints, 0.00001f, 0.00001f);

    const float fractions[] = { 0.0f, 0.37f, 1.0f };
    for (int i = 0; i < COUNT_OF(fractions); i++) {
        memcpy(expectedJoints, sourceJoints, sizeof(expectedJoints));
        memcpy(actualJoints, sourceJoints, sizeof(actualJoints));
        BE1::simdGeneric->AdditiveBlendJoints(expectedJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        simd4.AdditiveBlendJoints(actualJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        VerifyJointPoseArray(expectedJoints, actualJoints, numJoints, 0.0002f, 0.00001f);
    }
}
#endif

#if defined(HAVE_X86_AVX_INTRIN)
static void TestSIMD8AdditiveBlendJointsCorrectness() {
    static const int numJoints = 20;
    const int jointIndex[] = { 17, 3, 14, 0, 11, 6, 19, 2, 16, 8, 5, 13, 1 };
    const float fractions[] = { 0.0f, 0.37f, 1.0f };
    ALIGN_AS32 BE1::JointPose sourceJoints[numJoints];
    ALIGN_AS32 BE1::JointPose blendJoints[numJoints];
    ALIGN_AS32 BE1::JointPose expectedJoints[numJoints];
    ALIGN_AS32 BE1::JointPose actualJoints[numJoints];
    BE1::SIMD_8 simd8;

    RandomJointPoseArrayInit(sourceJoints, numJoints, false);
    RandomJointPoseArrayInit(blendJoints, numJoints, true);

    for (int i = 0; i < COUNT_OF(fractions); i++) {
        memcpy(expectedJoints, sourceJoints, sizeof(expectedJoints));
        memcpy(actualJoints, sourceJoints, sizeof(actualJoints));
        BE1::simdGeneric->AdditiveBlendJoints(expectedJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        simd8.AdditiveBlendJoints(actualJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        VerifyJointPoseArray(expectedJoints, actualJoints, numJoints, 0.0002f, 0.00001f);
    }
}
#endif

static void TestBlendJointsCorrectness(bool fast) {
    static const int numJoints = 20;
    const int jointIndex[] = { 17, 3, 14, 0, 11, 6, 19, 2, 16, 8, 5, 13, 1 };
    const float fractions[] = { 0.0f, 0.37f, 1.0f };
    ALIGN_AS32 BE1::JointPose sourceJoints[numJoints];
    ALIGN_AS32 BE1::JointPose blendJoints[numJoints];
    ALIGN_AS32 BE1::JointPose expectedJoints[numJoints];
    ALIGN_AS32 BE1::JointPose actualJoints[numJoints];

    RandomJointPoseArrayInit(sourceJoints, numJoints, false);
    RandomJointPoseArrayInit(blendJoints, numJoints, true);

    for (int i = 0; i < COUNT_OF(fractions); i++) {
        memcpy(expectedJoints, sourceJoints, sizeof(expectedJoints));
        memcpy(actualJoints, sourceJoints, sizeof(actualJoints));

        if (fast) {
            BE1::simdGeneric->BlendJointsFast(expectedJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        } else {
            BE1::simdGeneric->BlendJoints(expectedJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        }

        if (fast) {
            BE1::simdProcessor->BlendJointsFast(actualJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        } else {
            BE1::simdProcessor->BlendJoints(actualJoints, blendJoints, fractions[i], jointIndex, COUNT_OF(jointIndex));
        }

        VerifyJointPoseArray(expectedJoints, actualJoints, numJoints, 0.0002f, 0.00001f);
    }
}


static void TestBlendJoints() {
    TestBlendJointsCorrectness(false);

    uint64_t bestClocksGeneric = 0;
    uint64_t bestClocksSIMD = 0;

    ALIGN_AS32 BE1::JointPose sourceJoints[1024];
    ALIGN_AS32 BE1::JointPose blendJoints[1024];
    ALIGN_AS32 BE1::JointPose jointWork[1024];
    int jointIndex[1024];

    RandomJointPoseArrayInit(sourceJoints, COUNT_OF(sourceJoints), false);
    RandomJointPoseArrayInit(blendJoints, COUNT_OF(blendJoints), true);
    for (int i = 0; i < COUNT_OF(jointIndex); i++) {
        jointIndex[i] = i;
    }

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointWork, sourceJoints, sizeof(jointWork));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->BlendJoints(jointWork, blendJoints, 0.37f, jointIndex, COUNT_OF(jointIndex));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("BlendJoints 1024", bestClocksGeneric);

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointWork, sourceJoints, sizeof(jointWork));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->BlendJoints(jointWork, blendJoints, 0.37f, jointIndex, COUNT_OF(jointIndex));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("BlendJoints 1024", bestClocksGeneric, bestClocksSIMD);
}

static void TestBlendJointsFast() {
    TestBlendJointsCorrectness(true);

    uint64_t bestClocksGeneric = 0;
    uint64_t bestClocksSIMD = 0;

    ALIGN_AS32 BE1::JointPose sourceJoints[1024];
    ALIGN_AS32 BE1::JointPose blendJoints[1024];
    ALIGN_AS32 BE1::JointPose jointWork[1024];
    int jointIndex[1024];

    RandomJointPoseArrayInit(sourceJoints, COUNT_OF(sourceJoints), false);
    RandomJointPoseArrayInit(blendJoints, COUNT_OF(blendJoints), true);
    for (int i = 0; i < COUNT_OF(jointIndex); i++) {
        jointIndex[i] = i;
    }

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointWork, sourceJoints, sizeof(jointWork));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->BlendJointsFast(jointWork, blendJoints, 0.37f, jointIndex, COUNT_OF(jointIndex));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("BlendJointsFast 1024", bestClocksGeneric);

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointWork, sourceJoints, sizeof(jointWork));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->BlendJointsFast(jointWork, blendJoints, 0.37f, jointIndex, COUNT_OF(jointIndex));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("BlendJointsFast 1024", bestClocksGeneric, bestClocksSIMD);
}

static void TestConvertJointPosesToJointMats() {
    static const int numJoints = 1024;
    const int boundaryCounts[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17 };
    ALIGN_AS32 BE1::JointPose jointPoses[numJoints];
    ALIGN_AS32 BE1::Mat3x4 expectedJointMats[numJoints];
    ALIGN_AS32 BE1::Mat3x4 actualJointMats[numJoints];

    RandomJointPoseArrayInit(jointPoses, numJoints, false);

    for (int i = 0; i < COUNT_OF(boundaryCounts); i++) {
        const int count = boundaryCounts[i];
        BE1::simdGeneric->ConvertJointPosesToJointMats(expectedJointMats, jointPoses, count);
        BE1::simdProcessor->ConvertJointPosesToJointMats(actualJointMats, jointPoses, count);

        for (int j = 0; j < count; j++) {
            VERIFY_SIMD_PRIMITIVE(actualJointMats[j].Equals(expectedJointMats[j], 0.0001f));
        }
    }

    uint64_t bestClocksGeneric = 0;
    uint64_t bestClocksSIMD = 0;

    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->ConvertJointPosesToJointMats(expectedJointMats, jointPoses, numJoints);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("ConvertJointPosesToJointMats 1024", bestClocksGeneric);

    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->ConvertJointPosesToJointMats(actualJointMats, jointPoses, numJoints);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("ConvertJointPosesToJointMats 1024", bestClocksGeneric, bestClocksSIMD);
}

static void TestConvertJointMatsToJointPoses() {
    static const int numJoints = 1024;
    const int boundaryCounts[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17 };
    ALIGN_AS32 BE1::JointPose sourceJointPoses[numJoints];
    ALIGN_AS32 BE1::JointPose expectedJointPoses[numJoints];
    ALIGN_AS32 BE1::JointPose actualJointPoses[numJoints];
    ALIGN_AS32 BE1::Mat3x4 jointMats[numJoints];

    RandomJointPoseArrayInit(sourceJointPoses, numJoints, false);
    for (int i = 0; i < numJoints; i++) {
        switch (i & 3) {
        case 1: sourceJointPoses[i].q.Set(0.90f, 0.25f, -0.15f, 0.05f); sourceJointPoses[i].q.Normalize(); break;
        case 2: sourceJointPoses[i].q.Set(-0.20f, 0.92f, 0.18f, 0.08f); sourceJointPoses[i].q.Normalize(); break;
        case 3: sourceJointPoses[i].q.Set(0.15f, -0.22f, 0.94f, 0.07f); sourceJointPoses[i].q.Normalize(); break;
        }

        switch ((i >> 2) & 3) {
        case 1: sourceJointPoses[i].s.x = -sourceJointPoses[i].s.x; break;
        case 2: sourceJointPoses[i].s.x = -sourceJointPoses[i].s.x; sourceJointPoses[i].s.y = -sourceJointPoses[i].s.y; break;
        case 3: sourceJointPoses[i].s = -sourceJointPoses[i].s; break;
        }
    }
    BE1::simdGeneric->ConvertJointPosesToJointMats(jointMats, sourceJointPoses, numJoints);

    for (int i = 0; i < COUNT_OF(boundaryCounts); i++) {
        const int count = boundaryCounts[i];
        BE1::simdGeneric->ConvertJointMatsToJointPoses(expectedJointPoses, jointMats, count);
        BE1::simdProcessor->ConvertJointMatsToJointPoses(actualJointPoses, jointMats, count);

        for (int j = 0; j < count; j++) {
            const bool quatEquals = actualJointPoses[j].q.Equals(expectedJointPoses[j].q, 0.0002f) || (-actualJointPoses[j].q).Equals(expectedJointPoses[j].q, 0.0002f);
            VERIFY_SIMD_PRIMITIVE(quatEquals);
            VERIFY_SIMD_PRIMITIVE(actualJointPoses[j].t.Equals(expectedJointPoses[j].t, 0.0001f));
            VERIFY_SIMD_PRIMITIVE(actualJointPoses[j].s.Equals(expectedJointPoses[j].s, 0.0001f));
        }
    }

    uint64_t bestClocksGeneric = 0;
    uint64_t bestClocksSIMD = 0;

    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->ConvertJointMatsToJointPoses(expectedJointPoses, jointMats, numJoints);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("ConvertJointMatsToJointPoses 1024", bestClocksGeneric);

    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->ConvertJointMatsToJointPoses(actualJointPoses, jointMats, numJoints);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("ConvertJointMatsToJointPoses 1024", bestClocksGeneric, bestClocksSIMD);
}
#undef VERIFY_SIMD_PRIMITIVE

static void TestTransformJoints() {
    TestTransformJointsCorrectness();

    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 BE1::Mat3x4 jointMatrix[1024];
    int parents[1024];

    parents[0] = -1;
    for (int i = 1; i < COUNT_OF(parents); i++) {
        parents[i] = BE1::RNG::GetRandom(0, i - 1);
    }

    RandomFloatArrayInit((float *)jointMatrix, 12 * COUNT_OF(jointMatrix), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->TransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("TransformJoints", bestClocksGeneric);

    RandomFloatArrayInit((float *)jointMatrix, 12 * COUNT_OF(jointMatrix), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TestCount; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->TransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("TransformJoints", bestClocksGeneric, bestClocksSIMD);
}

static void TestUntransformJoints() {
    TestUntransformJointsCorrectness();

    uint64_t bestClocksGeneric = 0;
    uint64_t bestClocksSIMD = 0;
    ALIGN_AS32 BE1::Mat3x4 worldJointMatrix[1024];
    ALIGN_AS32 BE1::Mat3x4 jointMatrix[1024];
    int parents[1024];

    parents[0] = -1;
    worldJointMatrix[0] = BE1::Mat3x4::identity;
    for (int i = 1; i < COUNT_OF(parents); i++) {
        parents[i] = BE1::RNG::GetRandom(0, i - 1);
        worldJointMatrix[i] = BE1::Mat3x4::identity;
        worldJointMatrix[i].SetTranslation(BE1::Vec3(
            BE1::RNG::GetRandom(-100.0f, 100.0f),
            BE1::RNG::GetRandom(-100.0f, 100.0f),
            BE1::RNG::GetRandom(-100.0f, 100.0f)));
    }
    BE1::simdGeneric->TransformJoints(worldJointMatrix, parents, 0, COUNT_OF(worldJointMatrix) - 1);

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointMatrix, worldJointMatrix, sizeof(jointMatrix));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->UntransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("UntransformJoints", bestClocksGeneric);

    for (int i = 0; i < TestCount; i++) {
        memcpy(jointMatrix, worldJointMatrix, sizeof(jointMatrix));

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->UntransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("UntransformJoints", bestClocksGeneric, bestClocksSIMD);
}

void Test::TestSIMD() {
    BE_LOG("Testing SIMD processors..\n");

#if defined(HAVE_X86_SSE_INTRIN)
    TestSSEPrimitives();
#endif
#if defined(HAVE_X86_AVX_INTRIN)
    TestAVXPrimitives();
#endif
#if defined(HAVE_ARM_NEON_INTRIN)
    TestNEONPrimitives();
#endif

    TestAdd();
    TestSub();
    TestMul();
    TestDiv();
    TestSum();
    TestTransformPoints();
    TestTransformDirections();
#if defined(HAVE_X86_AVX_INTRIN)
    TestTranspose8x8();
#endif
    TestMemcpy();
    TestMemset();
#if defined(HAVE_X86_SSE_INTRIN) || defined(HAVE_ARM_NEON_INTRIN)
    TestSIMD4JointOpsCorrectness();
#endif
#if defined(HAVE_X86_AVX_INTRIN)
    TestSIMD8AdditiveBlendJointsCorrectness();
#endif
    TestBlendJoints();
    TestBlendJointsFast();
    TestConvertJointPosesToJointMats();
    TestConvertJointMatsToJointPoses();
    TestTransformJoints();
    TestUntransformJoints();
}
