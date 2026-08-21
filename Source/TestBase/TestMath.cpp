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
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits>

#if defined(NDEBUG)

constexpr int InvSqrtBenchmarkValueCount = 4096;
constexpr int InvSqrtBenchmarkIterationCount = 4096;
constexpr int InvSqrtBenchmarkSampleCount = 9;
constexpr int InvSqrtBenchmarkWarmupIterationCount = 64;

struct InvSqrtBenchmarkResult {
    double bestNanosecondsPerCall;
    double medianNanosecondsPerCall;
    float checksum;
};

static void InitializeInvSqrtBenchmarkInputs(float *inputs) {
    uint32_t randomState = 0x6d2b79f5u;

    for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
        randomState = randomState * 1664525u + 1013904223u;

        const float mantissa = 1.0f + static_cast<float>(randomState & 0xffffu) * (1.0f / 65536.0f);
        const int exponent = static_cast<int>((randomState >> 16) % 41u) - 20;
        inputs[i] = std::ldexp(mantissa, exponent);
    }
}

template <typename Function>
static InvSqrtBenchmarkResult MeasureInvSqrt(const float *inputs, Function function) {
    alignas(64) float values[InvSqrtBenchmarkValueCount];
    double samples[InvSqrtBenchmarkSampleCount];
    float checksum = 0.0f;

    for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
        values[i] = inputs[i];
    }

    for (int iteration = 0; iteration < InvSqrtBenchmarkWarmupIterationCount; iteration++) {
        for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
            values[i] = function(values[i]);
        }
    }

    for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
        checksum += values[i];
    }

    for (int sampleIndex = 0; sampleIndex < InvSqrtBenchmarkSampleCount; sampleIndex++) {
        for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
            values[i] = inputs[i];
        }

        const double startTime = BE1::PlatformTime::Seconds();

        // Each result becomes the next iteration's input so the optimizer cannot
        // replace the measured work with a single pass or remove dead stores.
        for (int iteration = 0; iteration < InvSqrtBenchmarkIterationCount; iteration++) {
            for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
                values[i] = function(values[i]);
            }
        }

        const double endTime = BE1::PlatformTime::Seconds();
        const double callCount = static_cast<double>(InvSqrtBenchmarkValueCount) * InvSqrtBenchmarkIterationCount;
        samples[sampleIndex] = (endTime - startTime) * 1.0e9 / callCount;

        for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
            checksum += values[i];
        }
    }

    std::sort(samples, samples + InvSqrtBenchmarkSampleCount);

    InvSqrtBenchmarkResult result;
    result.bestNanosecondsPerCall = samples[0];
    result.medianNanosecondsPerCall = samples[InvSqrtBenchmarkSampleCount / 2];
    result.checksum = checksum;
    return result;
}

template <typename Function>
static double MeasureInvSqrtMaxRelativeError(const float *inputs, Function function) {
    double maxRelativeError = 0.0;

    for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
        const double reference = 1.0 / std::sqrt(static_cast<double>(inputs[i]));
        const double result = static_cast<double>(function(inputs[i]));
        const double relativeError = std::abs(result - reference) / reference;
        maxRelativeError = std::max(maxRelativeError, relativeError);
    }

    return maxRelativeError;
}

template <typename Function>
static double MeasureSqrtMaxRelativeError(const float *inputs, Function function) {
    double maxRelativeError = 0.0;

    for (int i = 0; i < InvSqrtBenchmarkValueCount; i++) {
        const double reference = std::sqrt(static_cast<double>(inputs[i]));
        const double result = static_cast<double>(function(inputs[i]));
        const double relativeError = std::abs(result - reference) / reference;
        maxRelativeError = std::max(maxRelativeError, relativeError);
    }

    return maxRelativeError;
}

constexpr int TrigBenchmarkValueCount = 4096;
constexpr int TrigBenchmarkIterationCount = 2048;
constexpr int TrigBenchmarkSampleCount = 9;
constexpr int TrigBenchmarkWarmupIterationCount = 32;
constexpr int TrigBenchmarkLaneCount = 8;

struct TrigBenchmarkResult {
    double bestNanosecondsPerCall;
    double medianNanosecondsPerCall;
    float checksum;
};

struct SinCosMaxAbsoluteError {
    double sine;
    double cosine;
};

static float NextTrigBenchmarkInput(uint32_t &randomState, float minimum, float maximum) {
    randomState = randomState * 1664525u + 1013904223u;
    const float unit = static_cast<float>(randomState >> 8) * (1.0f / 16777216.0f);
    return minimum + (maximum - minimum) * unit;
}

static void InitializeTrigBenchmarkInputs(float *inputs, float minimum, float maximum, uint32_t randomState) {
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        inputs[i] = NextTrigBenchmarkInput(randomState, minimum, maximum);
    }
}

static void InitializeATanBenchmarkInputs(float *inputs, uint32_t randomState) {
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        randomState = randomState * 1664525u + 1013904223u;

        const float mantissa = 1.0f + static_cast<float>(randomState & 0xffffu) * (1.0f / 65536.0f);
        const int exponent = static_cast<int>((randomState >> 16) % 25u) - 12;
        const float magnitude = std::ldexp(mantissa, exponent);
        inputs[i] = (randomState & 0x80000000u) ? -magnitude : magnitude;
    }
}

static void InitializeTanBenchmarkInputs(float *inputs, uint32_t randomState) {
    constexpr float singularityMargin = 0.01f;

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        float input;
        do {
            input = NextTrigBenchmarkInput(randomState, -BE1::Math::Pi, BE1::Math::Pi);
        } while (std::abs(std::abs(input) - BE1::Math::HalfPi) < singularityMargin);
        inputs[i] = input;
    }

    inputs[0] = -BE1::Math::Pi;
    inputs[1] = 0.0f;
    inputs[2] = BE1::Math::Pi;
    inputs[3] = -BE1::Math::HalfPi - singularityMargin;
    inputs[4] = -BE1::Math::HalfPi + singularityMargin;
    inputs[5] = BE1::Math::HalfPi - singularityMargin;
    inputs[6] = BE1::Math::HalfPi + singularityMargin;
}

static void InitializeLogBenchmarkInputs(float *inputs, uint32_t randomState) {
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        randomState = randomState * 1664525u + 1013904223u;

        const float mantissa = 1.0f + static_cast<float>(randomState & 0xffffu) * (1.0f / 65536.0f);
        const int exponent = static_cast<int>((randomState >> 16) % 241u) - 120;
        inputs[i] = std::ldexp(mantissa, exponent);
    }

    inputs[0] = std::numeric_limits<float>::min();
    inputs[1] = std::numeric_limits<float>::max();
    inputs[2] = 0.5f;
    inputs[3] = 1.0f;
    inputs[4] = 2.0f;
    inputs[5] = 10.0f;
}

static void InitializeExpBenchmarkInputs(float *inputs, float minValue, float maxValue, uint32_t randomState) {
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        inputs[i] = NextTrigBenchmarkInput(randomState, minValue, maxValue);
    }

    inputs[0] = minValue;
    inputs[1] = -0.693147180559945f;
    inputs[2] = -1.0f;
    inputs[3] = 0.0f;
    inputs[4] = 1.0f;
    inputs[5] = 0.693147180559945f;
    inputs[6] = maxValue;
}

static void InitializePowBenchmarkInputs(float *baseInputs, float *exponentInputs, uint32_t randomState) {
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        const float logBase = NextTrigBenchmarkInput(randomState, -2.079441541679836f, 2.079441541679836f);
        baseInputs[i] = static_cast<float>(std::exp(static_cast<double>(logBase)));
        exponentInputs[i] = NextTrigBenchmarkInput(randomState, -8.0f, 8.0f);
    }

    baseInputs[0] = 0.125f;
    exponentInputs[0] = -8.0f;
    baseInputs[1] = 0.125f;
    exponentInputs[1] = 8.0f;
    baseInputs[2] = 0.5f;
    exponentInputs[2] = -2.0f;
    baseInputs[3] = 1.0f;
    exponentInputs[3] = 0.0f;
    baseInputs[4] = 2.0f;
    exponentInputs[4] = 0.5f;
    baseInputs[5] = 8.0f;
    exponentInputs[5] = -8.0f;
    baseInputs[6] = 8.0f;
    exponentInputs[6] = 8.0f;
}

template <typename Function>
static TrigBenchmarkResult MeasureTrigUnary(const float *inputs, Function function) {
    double samples[TrigBenchmarkSampleCount];
    float checksum = 0.0f;

    for (int iteration = 0; iteration < TrigBenchmarkWarmupIterationCount; iteration++) {
        for (int i = 0; i < TrigBenchmarkValueCount; i++) {
            checksum += function(inputs[(i + iteration) & (TrigBenchmarkValueCount - 1)]);
        }
    }

    for (int sampleIndex = 0; sampleIndex < TrigBenchmarkSampleCount; sampleIndex++) {
        float accumulators[TrigBenchmarkLaneCount] = {};

        const double startTime = BE1::PlatformTime::Seconds();

        // Rotate the fixed input set and use independent accumulators so calls
        // cannot be hoisted or removed without turning the benchmark into a
        // single serial accumulation chain.
        for (int iteration = 0; iteration < TrigBenchmarkIterationCount; iteration++) {
            for (int i = 0; i < TrigBenchmarkValueCount; i++) {
                const int lane = i & (TrigBenchmarkLaneCount - 1);
                const int inputIndex = (i + iteration) & (TrigBenchmarkValueCount - 1);
                accumulators[lane] += function(inputs[inputIndex]);
            }
        }

        const double endTime = BE1::PlatformTime::Seconds();
        const double callCount = static_cast<double>(TrigBenchmarkValueCount) * TrigBenchmarkIterationCount;
        samples[sampleIndex] = (endTime - startTime) * 1.0e9 / callCount;

        for (int lane = 0; lane < TrigBenchmarkLaneCount; lane++) {
            checksum += accumulators[lane];
        }
    }

    std::sort(samples, samples + TrigBenchmarkSampleCount);

    TrigBenchmarkResult result;
    result.bestNanosecondsPerCall = samples[0];
    result.medianNanosecondsPerCall = samples[TrigBenchmarkSampleCount / 2];
    result.checksum = checksum;
    return result;
}

template <typename Function>
static TrigBenchmarkResult MeasureTrigBinary(const float *firstInputs, const float *secondInputs, Function function) {
    double samples[TrigBenchmarkSampleCount];
    float checksum = 0.0f;

    for (int iteration = 0; iteration < TrigBenchmarkWarmupIterationCount; iteration++) {
        for (int i = 0; i < TrigBenchmarkValueCount; i++) {
            const int inputIndex = (i + iteration) & (TrigBenchmarkValueCount - 1);
            checksum += function(firstInputs[inputIndex], secondInputs[inputIndex]);
        }
    }

    for (int sampleIndex = 0; sampleIndex < TrigBenchmarkSampleCount; sampleIndex++) {
        float accumulators[TrigBenchmarkLaneCount] = {};

        const double startTime = BE1::PlatformTime::Seconds();

        for (int iteration = 0; iteration < TrigBenchmarkIterationCount; iteration++) {
            for (int i = 0; i < TrigBenchmarkValueCount; i++) {
                const int lane = i & (TrigBenchmarkLaneCount - 1);
                const int inputIndex = (i + iteration) & (TrigBenchmarkValueCount - 1);
                accumulators[lane] += function(firstInputs[inputIndex], secondInputs[inputIndex]);
            }
        }

        const double endTime = BE1::PlatformTime::Seconds();
        const double callCount = static_cast<double>(TrigBenchmarkValueCount) * TrigBenchmarkIterationCount;
        samples[sampleIndex] = (endTime - startTime) * 1.0e9 / callCount;

        for (int lane = 0; lane < TrigBenchmarkLaneCount; lane++) {
            checksum += accumulators[lane];
        }
    }

    std::sort(samples, samples + TrigBenchmarkSampleCount);

    TrigBenchmarkResult result;
    result.bestNanosecondsPerCall = samples[0];
    result.medianNanosecondsPerCall = samples[TrigBenchmarkSampleCount / 2];
    result.checksum = checksum;
    return result;
}

template <typename Function>
static TrigBenchmarkResult MeasureTrigSinCos(const float *inputs, Function function) {
    double samples[TrigBenchmarkSampleCount];
    float checksum = 0.0f;

    for (int iteration = 0; iteration < TrigBenchmarkWarmupIterationCount; iteration++) {
        for (int i = 0; i < TrigBenchmarkValueCount; i++) {
            float sine;
            float cosine;
            function(inputs[(i + iteration) & (TrigBenchmarkValueCount - 1)], sine, cosine);
            checksum += sine + cosine;
        }
    }

    for (int sampleIndex = 0; sampleIndex < TrigBenchmarkSampleCount; sampleIndex++) {
        float sineAccumulators[TrigBenchmarkLaneCount] = {};
        float cosineAccumulators[TrigBenchmarkLaneCount] = {};

        const double startTime = BE1::PlatformTime::Seconds();

        for (int iteration = 0; iteration < TrigBenchmarkIterationCount; iteration++) {
            for (int i = 0; i < TrigBenchmarkValueCount; i++) {
                const int lane = i & (TrigBenchmarkLaneCount - 1);
                const int inputIndex = (i + iteration) & (TrigBenchmarkValueCount - 1);
                float sine;
                float cosine;
                function(inputs[inputIndex], sine, cosine);
                sineAccumulators[lane] += sine;
                cosineAccumulators[lane] += cosine;
            }
        }

        const double endTime = BE1::PlatformTime::Seconds();
        const double callCount = static_cast<double>(TrigBenchmarkValueCount) * TrigBenchmarkIterationCount;
        samples[sampleIndex] = (endTime - startTime) * 1.0e9 / callCount;

        for (int lane = 0; lane < TrigBenchmarkLaneCount; lane++) {
            checksum += sineAccumulators[lane] + cosineAccumulators[lane];
        }
    }

    std::sort(samples, samples + TrigBenchmarkSampleCount);

    TrigBenchmarkResult result;
    result.bestNanosecondsPerCall = samples[0];
    result.medianNanosecondsPerCall = samples[TrigBenchmarkSampleCount / 2];
    result.checksum = checksum;
    return result;
}

template <typename Function, typename Reference>
static double MeasureTrigUnaryMaxAbsoluteError(const float *inputs, Function function, Reference reference) {
    double maxAbsoluteError = 0.0;

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        const double expected = reference(static_cast<double>(inputs[i]));
        const double actual = static_cast<double>(function(inputs[i]));
        maxAbsoluteError = std::max(maxAbsoluteError, std::abs(actual - expected));
    }

    return maxAbsoluteError;
}

template <typename Function, typename Reference>
static double MeasureTrigUnaryMaxRelativeError(const float *inputs, Function function, Reference reference) {
    double maxRelativeError = 0.0;

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        const double expected = reference(static_cast<double>(inputs[i]));
        const double actual = static_cast<double>(function(inputs[i]));
        const double relativeError = expected != 0.0 ? std::abs((actual - expected) / expected) : std::abs(actual);
        maxRelativeError = std::max(maxRelativeError, relativeError);
    }

    return maxRelativeError;
}

template <typename Function, typename Reference>
static double MeasureTrigBinaryMaxAbsoluteError(const float *firstInputs, const float *secondInputs, Function function, Reference reference) {
    double maxAbsoluteError = 0.0;

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        const double expected = reference(static_cast<double>(firstInputs[i]), static_cast<double>(secondInputs[i]));
        const double actual = static_cast<double>(function(firstInputs[i], secondInputs[i]));
        maxAbsoluteError = std::max(maxAbsoluteError, std::abs(actual - expected));
    }

    return maxAbsoluteError;
}

template <typename Function, typename Reference>
static double MeasureTrigBinaryMaxRelativeError(const float *firstInputs, const float *secondInputs, Function function, Reference reference) {
    double maxRelativeError = 0.0;

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        const double expected = reference(static_cast<double>(firstInputs[i]), static_cast<double>(secondInputs[i]));
        const double actual = static_cast<double>(function(firstInputs[i], secondInputs[i]));
        const double relativeError = expected != 0.0 ? std::abs((actual - expected) / expected) : std::abs(actual);
        maxRelativeError = std::max(maxRelativeError, relativeError);
    }

    return maxRelativeError;
}

template <typename Function>
static SinCosMaxAbsoluteError MeasureSinCosMaxAbsoluteError(const float *inputs, Function function) {
    SinCosMaxAbsoluteError maxAbsoluteError = {};

    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        float sine;
        float cosine;
        function(inputs[i], sine, cosine);
        const double input = static_cast<double>(inputs[i]);
        maxAbsoluteError.sine = std::max(maxAbsoluteError.sine, std::abs(static_cast<double>(sine) - std::sin(input)));
        maxAbsoluteError.cosine = std::max(maxAbsoluteError.cosine, std::abs(static_cast<double>(cosine) - std::cos(input)));
    }

    return maxAbsoluteError;
}

template <typename Function, typename FastFunction, typename Reference>
static void RunTrigUnaryBenchmark(const char *name, const char *fastName, const float *inputs,
    Function function, FastFunction fastFunction, Reference reference, bool resultIsAngle) {
    const TrigBenchmarkResult result = MeasureTrigUnary(inputs, function);
    const TrigBenchmarkResult fastResult = MeasureTrigUnary(inputs, fastFunction);
    const double maxAbsoluteError = MeasureTrigUnaryMaxAbsoluteError(inputs, function, reference);
    const double fastMaxAbsoluteError = MeasureTrigUnaryMaxAbsoluteError(inputs, fastFunction, reference);
    const double speedup = result.medianNanosecondsPerCall / fastResult.medianNanosecondsPerCall;

    BE_LOG("%s benchmark: %d values x %d iterations, median of %d samples\n", fastName, TrigBenchmarkValueCount, TrigBenchmarkIterationCount, TrigBenchmarkSampleCount);
    if (resultIsAngle) {
        const double radiansToDegrees = 180.0 / static_cast<double>(BE1::Math::Pi);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e rad (%.6f deg), checksum %.9g\n",
            name, result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxAbsoluteError, maxAbsoluteError * radiansToDegrees, result.checksum);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e rad (%.6f deg), checksum %.9g\n",
            fastName, fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxAbsoluteError, fastMaxAbsoluteError * radiansToDegrees, fastResult.checksum);
    } else {
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e, checksum %.9g\n",
            name, result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxAbsoluteError, result.checksum);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e, checksum %.9g\n",
            fastName, fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxAbsoluteError, fastResult.checksum);
    }
    BE_LOG("  %s speed: %.2fx compared with %s\n", fastName, speedup, name);
}

template <typename Function, typename FastFunction, typename Reference>
static void RunTrigBinaryBenchmark(const char *name, const char *fastName, const float *firstInputs, const float *secondInputs,
    Function function, FastFunction fastFunction, Reference reference, bool resultIsAngle) {
    const TrigBenchmarkResult result = MeasureTrigBinary(firstInputs, secondInputs, function);
    const TrigBenchmarkResult fastResult = MeasureTrigBinary(firstInputs, secondInputs, fastFunction);
    const double maxAbsoluteError = MeasureTrigBinaryMaxAbsoluteError(firstInputs, secondInputs, function, reference);
    const double fastMaxAbsoluteError = MeasureTrigBinaryMaxAbsoluteError(firstInputs, secondInputs, fastFunction, reference);
    const double speedup = result.medianNanosecondsPerCall / fastResult.medianNanosecondsPerCall;

    BE_LOG("%s benchmark: %d values x %d iterations, median of %d samples\n", fastName, TrigBenchmarkValueCount, TrigBenchmarkIterationCount, TrigBenchmarkSampleCount);
    if (resultIsAngle) {
        const double radiansToDegrees = 180.0 / static_cast<double>(BE1::Math::Pi);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e rad (%.6f deg), checksum %.9g\n",
            name, result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxAbsoluteError, maxAbsoluteError * radiansToDegrees, result.checksum);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e rad (%.6f deg), checksum %.9g\n",
            fastName, fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxAbsoluteError, fastMaxAbsoluteError * radiansToDegrees, fastResult.checksum);
    } else {
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e, checksum %.9g\n",
            name, result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxAbsoluteError, result.checksum);
        BE_LOG("  %-11s %.3f ns/call (best %.3f), max absolute error %.3e, checksum %.9g\n",
            fastName, fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxAbsoluteError, fastResult.checksum);
    }
    BE_LOG("  %s speed: %.2fx compared with %s\n", fastName, speedup, name);
}

template <typename Function, typename FastFunction>
static void RunSinCosBenchmark(const float *inputs, Function function, FastFunction fastFunction) {
    const TrigBenchmarkResult result = MeasureTrigSinCos(inputs, function);
    const TrigBenchmarkResult fastResult = MeasureTrigSinCos(inputs, fastFunction);
    const SinCosMaxAbsoluteError maxAbsoluteError = MeasureSinCosMaxAbsoluteError(inputs, function);
    const SinCosMaxAbsoluteError fastMaxAbsoluteError = MeasureSinCosMaxAbsoluteError(inputs, fastFunction);
    const double speedup = result.medianNanosecondsPerCall / fastResult.medianNanosecondsPerCall;

    BE_LOG("SinCosFast benchmark: %d values x %d iterations, median of %d samples\n", TrigBenchmarkValueCount, TrigBenchmarkIterationCount, TrigBenchmarkSampleCount);
    BE_LOG("  SinCos      %.3f ns/call (best %.3f), max absolute error (sin %.3e, cos %.3e), checksum %.9g\n",
        result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxAbsoluteError.sine, maxAbsoluteError.cosine, result.checksum);
    BE_LOG("  SinCosFast  %.3f ns/call (best %.3f), max absolute error (sin %.3e, cos %.3e), checksum %.9g\n",
        fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxAbsoluteError.sine, fastMaxAbsoluteError.cosine, fastResult.checksum);
    BE_LOG("  SinCosFast speed: %.2fx compared with SinCos\n", speedup);
}

#endif

static void BenchmarkInvSqrt() {
#if defined(NDEBUG)
    alignas(64) float inputs[InvSqrtBenchmarkValueCount];
    InitializeInvSqrtBenchmarkInputs(inputs);

    const auto invSqrt = [](float x) { return BE1::Math::InvSqrt(x); };
    const auto invSqrtFast = [](float x) { return BE1::Math::InvSqrtFast(x); };

    const InvSqrtBenchmarkResult invSqrtResult = MeasureInvSqrt(inputs, invSqrt);
    const InvSqrtBenchmarkResult invSqrtFastResult = MeasureInvSqrt(inputs, invSqrtFast);
    const double invSqrtMaxRelativeError = MeasureInvSqrtMaxRelativeError(inputs, invSqrt);
    const double invSqrtFastMaxRelativeError = MeasureInvSqrtMaxRelativeError(inputs, invSqrtFast);
    const double speedup = invSqrtResult.medianNanosecondsPerCall / invSqrtFastResult.medianNanosecondsPerCall;

    BE_LOG("InvSqrtFast benchmark: %d values x %d iterations, median of %d samples\n", InvSqrtBenchmarkValueCount, InvSqrtBenchmarkIterationCount, InvSqrtBenchmarkSampleCount);
    BE_LOG("  InvSqrt: %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n",
        invSqrtResult.medianNanosecondsPerCall, invSqrtResult.bestNanosecondsPerCall, invSqrtMaxRelativeError, invSqrtResult.checksum);
    BE_LOG("  InvSqrtFast: %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n",
        invSqrtFastResult.medianNanosecondsPerCall, invSqrtFastResult.bestNanosecondsPerCall, invSqrtFastMaxRelativeError, invSqrtFastResult.checksum);
    BE_LOG("  InvSqrtFast speed: %.2fx compared with InvSqrt\n", speedup);
#else
    BE_LOG("InvSqrtFast benchmark requires an optimized Release build; skipped in this build.\n");
#endif
}

static void BenchmarkSqrt() {
#if defined(NDEBUG)
    alignas(64) float inputs[InvSqrtBenchmarkValueCount];
    InitializeInvSqrtBenchmarkInputs(inputs);

    const auto sqrt = [](float x) { return BE1::Math::Sqrt(x); };
    const auto sqrtFast = [](float x) { return BE1::Math::SqrtFast(x); };

    const InvSqrtBenchmarkResult sqrtResult = MeasureInvSqrt(inputs, sqrt);
    const InvSqrtBenchmarkResult sqrtFastResult = MeasureInvSqrt(inputs, sqrtFast);
    const double sqrtMaxRelativeError = MeasureSqrtMaxRelativeError(inputs, sqrt);
    const double sqrtFastMaxRelativeError = MeasureSqrtMaxRelativeError(inputs, sqrtFast);

    BE_LOG("SqrtFast benchmark: %d values x %d iterations, median of %d samples\n", InvSqrtBenchmarkValueCount, InvSqrtBenchmarkIterationCount, InvSqrtBenchmarkSampleCount);
    BE_LOG("  Sqrt:     %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n",
        sqrtResult.medianNanosecondsPerCall, sqrtResult.bestNanosecondsPerCall, sqrtMaxRelativeError, sqrtResult.checksum);
    BE_LOG("  SqrtFast: %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n",
        sqrtFastResult.medianNanosecondsPerCall, sqrtFastResult.bestNanosecondsPerCall, sqrtFastMaxRelativeError, sqrtFastResult.checksum);
    BE_LOG("  SqrtFast speed: %.2fx compared with Sqrt\n", sqrtResult.medianNanosecondsPerCall / sqrtFastResult.medianNanosecondsPerCall);
#else
    BE_LOG("SqrtFast benchmark requires an optimized Release build; skipped in this build.\n");
#endif
}


static void BenchmarkTrigonometry() {
#if defined(NDEBUG)
    alignas(64) float angleInputs[TrigBenchmarkValueCount];
    alignas(64) float tanInputs[TrigBenchmarkValueCount];
    alignas(64) float unitInputs[TrigBenchmarkValueCount];
    alignas(64) float atanInputs[TrigBenchmarkValueCount];
    alignas(64) float atanYInputs[TrigBenchmarkValueCount];
    alignas(64) float atanXInputs[TrigBenchmarkValueCount];

    InitializeTrigBenchmarkInputs(angleInputs, -BE1::Math::TwoPi, BE1::Math::TwoPi, 0x243f6a88u);
    InitializeTanBenchmarkInputs(tanInputs, 0x85a308d3u);
    InitializeTrigBenchmarkInputs(unitInputs, -1.0f, 1.0f, 0x13198a2eu);
    InitializeATanBenchmarkInputs(atanInputs, 0x68bc21ebu);
    InitializeATanBenchmarkInputs(atanYInputs, 0x02e5be93u);
    InitializeATanBenchmarkInputs(atanXInputs, 0x967a889bu);

    // Guarantee an even distribution across all four atan2 quadrants.
    for (int i = 0; i < TrigBenchmarkValueCount; i++) {
        atanYInputs[i] = (i & 2) ? -std::abs(atanYInputs[i]) : std::abs(atanYInputs[i]);
        atanXInputs[i] = ((i + 1) & 2) ? -std::abs(atanXInputs[i]) : std::abs(atanXInputs[i]);
    }

    angleInputs[0] = -BE1::Math::TwoPi;
    angleInputs[1] = -BE1::Math::Pi;
    angleInputs[2] = -BE1::Math::HalfPi;
    angleInputs[3] = 0.0f;
    angleInputs[4] = BE1::Math::HalfPi;
    angleInputs[5] = BE1::Math::Pi;
    angleInputs[6] = BE1::Math::TwoPi;
    unitInputs[0] = -1.0f;
    unitInputs[1] = 0.0f;
    unitInputs[2] = 1.0f;

    RunTrigUnaryBenchmark("Sin", "SinFast", angleInputs,
        [](float a) { return BE1::Math::Sin(a); },
        [](float a) { return BE1::Math::SinFast(a); },
        [](double a) { return std::sin(a); }, false);

    RunTrigUnaryBenchmark("Cos", "CosFast", angleInputs,
        [](float a) { return BE1::Math::Cos(a); },
        [](float a) { return BE1::Math::CosFast(a); },
        [](double a) { return std::cos(a); }, false);

    RunSinCosBenchmark(angleInputs,
        [](float a, float &sine, float &cosine) { BE1::Math::SinCos(a, sine, cosine); },
        [](float a, float &sine, float &cosine) { BE1::Math::SinCosFast(a, sine, cosine); });

    RunTrigUnaryBenchmark("Tan", "TanFast", tanInputs,
        [](float a) { return BE1::Math::Tan(a); },
        [](float a) { return BE1::Math::TanFast(a); },
        [](double a) { return std::tan(a); }, false);

    RunTrigUnaryBenchmark("ASin", "ASinFast", unitInputs,
        [](float a) { return BE1::Math::ASin(a); },
        [](float a) { return BE1::Math::ASinFast(a); },
        [](double a) { return std::asin(a); }, true);

    RunTrigUnaryBenchmark("ACos", "ACosFast", unitInputs,
        [](float a) { return BE1::Math::ACos(a); },
        [](float a) { return BE1::Math::ACosFast(a); },
        [](double a) { return std::acos(a); }, true);

    RunTrigUnaryBenchmark("ATan", "ATanFast", atanInputs,
        [](float a) { return BE1::Math::ATan(a); },
        [](float a) { return BE1::Math::ATanFast(a); },
        [](double a) { return std::atan(a); }, true);

    RunTrigBinaryBenchmark("ATan2", "ATan2Fast", atanYInputs, atanXInputs,
        [](float y, float x) { return BE1::Math::ATan2(y, x); },
        [](float y, float x) { return BE1::Math::ATan2Fast(y, x); },
        [](double y, double x) { return std::atan2(y, x); }, true);

#else
    BE_LOG("Fast trigonometric benchmarks require an optimized Release build; skipped in this build.\n");
#endif
}

static void BenchmarkLog() {
#if defined(NDEBUG)
    alignas(64) float inputs[TrigBenchmarkValueCount];
    InitializeLogBenchmarkInputs(inputs, 0xa4093822u);

    RunTrigUnaryBenchmark("Log", "LogFast", inputs,
        [](float f) { return BE1::Math::Log(f); },
        [](float f) { return BE1::Math::LogFast(f); },
        [](double f) { return std::log(f); }, false);

    RunTrigUnaryBenchmark("Log2", "Log2Fast", inputs,
        [](float f) { return BE1::Math::Log2(f); },
        [](float f) { return BE1::Math::Log2Fast(f); },
        [](double f) { return std::log2(f); }, false);

    RunTrigUnaryBenchmark("Log10", "Log10Fast", inputs,
        [](float f) { return BE1::Math::Log10(f); },
        [](float f) { return BE1::Math::Log10Fast(f); },
        [](double f) { return std::log10(f); }, false);
#else
    BE_LOG("LogFast benchmark requires an optimized Release build; skipped in this build.\n");
#endif
}

static void BenchmarkExp() {
#if defined(NDEBUG)
    alignas(64) float inputs[TrigBenchmarkValueCount];
    alignas(64) float accuracyInputs[TrigBenchmarkValueCount];
    InitializeExpBenchmarkInputs(inputs, -10.0f, 10.0f, 0x299f31d0u);
    InitializeExpBenchmarkInputs(accuracyInputs, -87.0f, 87.0f, 0x082efa98u);

    const auto expFunction = [](float f) { return BE1::Math::Exp(f); };
    const auto expFastFunction = [](float f) { return BE1::Math::ExpFast(f); };
    const auto reference = [](double f) { return std::exp(f); };

    const TrigBenchmarkResult result = MeasureTrigUnary(inputs, expFunction);
    const TrigBenchmarkResult fastResult = MeasureTrigUnary(inputs, expFastFunction);
    const double maxRelativeError = MeasureTrigUnaryMaxRelativeError(accuracyInputs, expFunction, reference);
    const double fastMaxRelativeError = MeasureTrigUnaryMaxRelativeError(accuracyInputs, expFastFunction, reference);

    BE_LOG("ExpFast benchmark: %d values x %d iterations, median of %d samples; error measured over [-87, 87]\n", TrigBenchmarkValueCount, TrigBenchmarkIterationCount, TrigBenchmarkSampleCount);
    BE_LOG("  %-11s %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n", "Exp", result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxRelativeError, result.checksum);
    BE_LOG("  %-11s %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n", "ExpFast", fastResult.medianNanosecondsPerCall, fastResult.bestNanosecondsPerCall, fastMaxRelativeError, fastResult.checksum);
    BE_LOG("  ExpFast speed: %.2fx compared with Exp\n", result.medianNanosecondsPerCall / fastResult.medianNanosecondsPerCall);
#else
    BE_LOG("ExpFast benchmark requires an optimized Release build; skipped in this build.\n");
#endif
}

static void BenchmarkPow() {
#if defined(NDEBUG)
    alignas(64) float baseInputs[TrigBenchmarkValueCount];
    alignas(64) float exponentInputs[TrigBenchmarkValueCount];
    InitializePowBenchmarkInputs(baseInputs, exponentInputs, 0xec4e6c89u);

    const auto powFunction = [](float x, float y) { return BE1::Math::Pow(x, y); };
    const auto powFastFunction = [](float x, float y) { return BE1::Math::PowFast(x, y); };
    const auto reference = [](double x, double y) { return std::pow(x, y); };

    const TrigBenchmarkResult result = MeasureTrigBinary(baseInputs, exponentInputs, powFunction);
    const TrigBenchmarkResult powFastResult = MeasureTrigBinary(baseInputs, exponentInputs, powFastFunction);
    const double maxRelativeError = MeasureTrigBinaryMaxRelativeError(baseInputs, exponentInputs, powFunction, reference);
    const double powFastMaxRelativeError = MeasureTrigBinaryMaxRelativeError(baseInputs, exponentInputs, powFastFunction, reference);

    BE_LOG("PowFast benchmark: %d values x %d iterations, median of %d samples; positive bases in [0.125, 8], exponents in [-8, 8]\n", TrigBenchmarkValueCount, TrigBenchmarkIterationCount, TrigBenchmarkSampleCount);
    BE_LOG("  %-11s %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n", "Pow", result.medianNanosecondsPerCall, result.bestNanosecondsPerCall, maxRelativeError, result.checksum);
    BE_LOG("  %-11s %.3f ns/call (best %.3f), max relative error %.3e, checksum %.9g\n", "PowFast", powFastResult.medianNanosecondsPerCall, powFastResult.bestNanosecondsPerCall, powFastMaxRelativeError, powFastResult.checksum);
    BE_LOG("  PowFast speed: %.2fx compared with Pow\n", result.medianNanosecondsPerCall / powFastResult.medianNanosecondsPerCall);
#else
    BE_LOG("PowFast benchmark requires an optimized Release build; skipped in this build.\n");
#endif
}

static void VerifyMathTest(bool expression, const char *testName, int line) {
    if (!expression) {
        std::fprintf(stderr, "%s verification failed at line %d\n", testName, line);
        BE_FATALERROR("%s verification failed at line %d\n", testName, line);
        std::abort();
    }
}

#define VERIFY_INTEGER_LOG2(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestIntegerLog2() {
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(1.0f) == 0);
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(0.5f) == -1);
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(std::numeric_limits<float>::min()) == -126);

    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(1) == 0);
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(2) == 1);
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(3) == 1);
    VERIFY_INTEGER_LOG2(BE1::Math::ILog2(std::numeric_limits<int>::max()) == 30);

    VERIFY_INTEGER_LOG2(BE1::Math::BitsForFloat(0.5f) == 1);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForFloat(3.0f) == 2);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForFloat(3.1f) == 3);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForFloat(4.0f) == 3);

    VERIFY_INTEGER_LOG2(BE1::Math::BitsForInteger(1) == 1);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForInteger(2) == 2);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForInteger(3) == 2);
    VERIFY_INTEGER_LOG2(BE1::Math::BitsForInteger(std::numeric_limits<int>::max()) == 31);
}

#undef VERIFY_INTEGER_LOG2

#define VERIFY_SQUARE_ROOT_CONTRACT(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestSquareRootContract() {
    const float floatInfinity = std::numeric_limits<float>::infinity();
    const float floatNaN = std::numeric_limits<float>::quiet_NaN();
    const double doubleInfinity = std::numeric_limits<double>::infinity();
    const double doubleNaN = std::numeric_limits<double>::quiet_NaN();

    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(4.0f) == 2.0f);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(-1.0f) == 0.0f);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(floatNaN) == 0.0f);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(4.0) == 2.0);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(-1.0) == 0.0);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::Sqrt(doubleNaN) == 0.0);

    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(4.0f) == 0.5f);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(floatInfinity) == 0.0f);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(0.0f) == floatInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(-1.0f) == floatInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(std::numeric_limits<float>::denorm_min()) == floatInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(floatNaN) == floatInfinity);

    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(4.0) == 0.5);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(doubleInfinity) == 0.0);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(0.0) == doubleInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(-1.0) == doubleInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(std::numeric_limits<double>::denorm_min()) == doubleInfinity);
    VERIFY_SQUARE_ROOT_CONTRACT(BE1::Math::InvSqrt(doubleNaN) == doubleInfinity);
}

#undef VERIFY_SQUARE_ROOT_CONTRACT

#define VERIFY_POLYNOMIAL_SOLVER(expression) VerifyMathTest((expression), __func__, __LINE__)

static bool PolynomialRootEquals(float value, float expected, float tolerance = 1e-5f) {
    const float scale = std::max(1e-6f, std::abs(expected));
    return std::abs(value - expected) <= tolerance * scale;
}

static bool CubicResidualIsSmall(float a, float b, float c, float d, float root, float tolerance = 1e-5f) {
    const double x = root;
    const double residual = ((static_cast<double>(a) * x + b) * x + c) * x + d;
    const double scale = std::abs(static_cast<double>(a) * x * x * x) +
        std::abs(static_cast<double>(b) * x * x) + std::abs(static_cast<double>(c) * x) + std::abs(d);
    return std::abs(residual) <= tolerance * std::max(1.0, scale);
}

static void TestPolynomialSolvers() {
    static_assert(BE1::Min4(4, 1, 3, 2) == 1);
    static_assert(BE1::Max4(4, 1, 3, 2) == 4);
    static_assert(BE1::Min4Index(4, 1, 3, 2) == 1);
    static_assert(BE1::Max4Index(4, 1, 3, 2) == 0);
    static_assert(BE1::Min4Index(3, 1, 1, 2) == 2);
    static_assert(BE1::Max4Index(3, 4, 4, 2) == 2);
    static_assert(BE1::AbsMin(-4, 2) == 2);
    static_assert(BE1::AbsMax(-4, 2) == 4);
    static_assert(BE1::AbsMin3(-4, -2, 3) == 2);
    static_assert(BE1::AbsMax3(-4, -2, 3) == 4);
    static_assert(BE1::AbsMin4(-4, -2, 3, -1) == 1);
    static_assert(BE1::AbsMax4(-4, -2, 3, -1) == 4);

    float x1 = 7.0f;
    float x2 = 8.0f;
    float x3 = 9.0f;

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, -5.0f, 6.0f, x1, x2) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 3.0f));

    x2 = 8.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, 2.0f, 1.0f, x1, x2) == 1);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, -1.0f));
    VERIFY_POLYNOMIAL_SOLVER(x2 == 8.0f);

    x1 = 7.0f;
    x2 = 8.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, 0.0f, 1.0f, x1, x2) == 0);
    VERIFY_POLYNOMIAL_SOLVER(x1 == 7.0f && x2 == 8.0f);

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(0.0f, 2.0f, -4.0f, x1, x2) == 1);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 2.0f));

    x1 = 7.0f;
    x2 = 8.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1e-20f, -5e-20f, 6e-20f, x1, x2) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 3.0f));

    // 근의 공식을 float로 직접 계산하면 catastrophic cancellation으로 작은 근을 잃는다.
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, -10000.0f, 1.0f, x1, x2) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 0.0001f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 10000.0f));

    // 작은 최고차항도 멀리 떨어진 유효한 근을 만들 수 있으므로 선형식으로 강등하지 않는다.
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, -4000.0f, 3999999.0f, x1, x2) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 1999.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 2001.0f));

    x1 = 7.0f;
    x2 = 8.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, 2.0f, 1.0000001f, x1, x2) == 0);
    VERIFY_POLYNOMIAL_SOLVER(x1 == 7.0f && x2 == 8.0f);

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, -2.0f, 0.9999999f, x1, x2, 1e-3f) == 1);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 1.0f));

    x1 = 7.0f;
    x2 = 8.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(0.0f, 0.0f, 1.0f, x1, x2) == 0);
    VERIFY_POLYNOMIAL_SOLVER(x1 == 7.0f && x2 == 8.0f);
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealQuadratic(1.0f, 0.0f, -1.0f, x1, x2, 0.0f) == 0);
    VERIFY_POLYNOMIAL_SOLVER(x1 == 7.0f && x2 == 8.0f);

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1.0f, -6.0f, 11.0f, -6.0f, x1, x2, x3) == 3);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 1.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x3, 3.0f));

    x2 = 8.0f;
    x3 = 9.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1.0f, -3.0f, 3.0f, -1.0f, x1, x2, x3) == 1);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 1.0f));
    VERIFY_POLYNOMIAL_SOLVER(x2 == 8.0f && x3 == 9.0f);

    x3 = 9.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1.0f, 0.0f, -3.0f, 2.0f, x1, x2, x3) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, -2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 1.0f));
    VERIFY_POLYNOMIAL_SOLVER(x3 == 9.0f);

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1.0f, 0.0f, 1.0f, 1.0f, x1, x2, x3) == 1);
    VERIFY_POLYNOMIAL_SOLVER(CubicResidualIsSmall(1.0f, 0.0f, 1.0f, 1.0f, x1));

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1.0f, 0.0f, 100000.0f, -1.0f, x1, x2, x3) == 1);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 0.00001f));
    VERIFY_POLYNOMIAL_SOLVER(CubicResidualIsSmall(1.0f, 0.0f, 100000.0f, -1.0f, x1));

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(0.0f, 1.0f, -5.0f, 6.0f, x1, x2, x3) == 2);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 3.0f));

    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(1e-20f, -6e-20f, 11e-20f, -6e-20f, x1, x2, x3) == 3);
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x1, 1.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x2, 2.0f));
    VERIFY_POLYNOMIAL_SOLVER(PolynomialRootEquals(x3, 3.0f));

    x1 = 7.0f;
    x2 = 8.0f;
    x3 = 9.0f;
    VERIFY_POLYNOMIAL_SOLVER(BE1::Math::SolveRealCubic(INFINITY, 0.0f, 0.0f, 0.0f, x1, x2, x3) == 0);
    VERIFY_POLYNOMIAL_SOLVER(x1 == 7.0f && x2 == 8.0f && x3 == 9.0f);
}

#undef VERIFY_POLYNOMIAL_SOLVER

#define VERIFY_SPHERE_INTERSECTION(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestSphereIntersections() {
    const BE1::Sphere sphere(BE1::Vec3(0.0f, 0.0f, 0.0f), 1.0f);

    VERIFY_SPHERE_INTERSECTION(sphere.IsIntersectLine(BE1::Vec3(-2.0f, 0.0f, 0.0f), BE1::Vec3(2.0f, 0.0f, 0.0f)));
    VERIFY_SPHERE_INTERSECTION(!sphere.IsIntersectLine(BE1::Vec3(-2.0f, 2.0f, 0.0f), BE1::Vec3(2.0f, 2.0f, 0.0f)));
    VERIFY_SPHERE_INTERSECTION(sphere.IsIntersectLine(BE1::Vec3(0.5f, 0.0f, 0.0f), BE1::Vec3(0.5f, 0.0f, 0.0f)));
    VERIFY_SPHERE_INTERSECTION(!sphere.IsIntersectLine(BE1::Vec3(2.0f, 0.0f, 0.0f), BE1::Vec3(2.0f, 0.0f, 0.0f)));

    const BE1::Ray ray(BE1::Vec3(0.0f, 0.0f, 0.0f), BE1::Vec3(1.0f, 0.0f, 0.0f));
    const BE1::Sphere farSphere(BE1::Vec3(100000.0f, 0.0f, 0.0f), 1.0f);
    float hitDistMin = -1.0f;
    float hitDistMax = -1.0f;
    VERIFY_SPHERE_INTERSECTION(farSphere.IntersectRay(ray, &hitDistMin, &hitDistMax));
    VERIFY_SPHERE_INTERSECTION(PolynomialRootEquals(hitDistMin, 99999.0f));
    VERIFY_SPHERE_INTERSECTION(PolynomialRootEquals(hitDistMax, 100001.0f));

    const BE1::Sphere farTangentSphere(BE1::Vec3(100000.0f, 1.0f, 0.0f), 1.0f);
    VERIFY_SPHERE_INTERSECTION(farTangentSphere.IntersectRay(ray, &hitDistMin, &hitDistMax));
    VERIFY_SPHERE_INTERSECTION(PolynomialRootEquals(hitDistMin, 100000.0f));
    VERIFY_SPHERE_INTERSECTION(PolynomialRootEquals(hitDistMax, 100000.0f));

    const BE1::Sphere farMissSphere(BE1::Vec3(100000.0f, 2.0f, 0.0f), 1.0f);
    VERIFY_SPHERE_INTERSECTION(!farMissSphere.IntersectRay(ray, nullptr, nullptr));
}

#undef VERIFY_SPHERE_INTERSECTION

#define VERIFY_RANDOM(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestRandom() {
    BE1::RNG rng0(0x123456789abcdef0ull);
    BE1::RNG rng1(0x123456789abcdef0ull);

    float value00, value01;
    float value10, value11;
    rng0.GetNextGaussianPair(value00, value01);
    rng1.GetNextGaussianPair(value10, value11);

    VERIFY_RANDOM(std::isfinite(value00));
    VERIFY_RANDOM(std::isfinite(value01));
    VERIFY_RANDOM(value00 == value10);
    VERIFY_RANDOM(value01 == value11);

    // A Gaussian pair consumes exactly two values from the underlying generator.
    BE1::RNG sequenceRng(0x123456789abcdef0ull);
    sequenceRng.GetNextFloat();
    sequenceRng.GetNextFloat();
    VERIFY_RANDOM(rng0.GetNextUInt() == sequenceRng.GetNextUInt());

    // Scalar generation reuses the second result without advancing the generator.
    BE1::RNG scalarRng(0x123456789abcdef0ull);
    VERIFY_RANDOM(scalarRng.GetNextGaussian() == value00);
    VERIFY_RANDOM(scalarRng.GetNextGaussian() == value01);
    VERIFY_RANDOM(scalarRng.GetNextUInt() == rng1.GetNextUInt());

    // Reseeding invalidates a pending Gaussian value.
    BE1::RNG reseededRng(0x123456789abcdef0ull);
    const float firstValue = reseededRng.GetNextGaussian();
    reseededRng.SetSeed(0x123456789abcdef0ull);
    VERIFY_RANDOM(reseededRng.GetNextGaussian() == firstValue);

    // Scalar and pair calls preserve the same Gaussian sequence when interleaved.
    BE1::RNG sequentialRng(0x123456789abcdef0ull);
    float sequentialValues[4];
    for (int i = 0; i < 4; i++) {
        sequentialValues[i] = sequentialRng.GetNextGaussian();
    }

    BE1::RNG mixedRng(0x123456789abcdef0ull);
    VERIFY_RANDOM(mixedRng.GetNextGaussian() == sequentialValues[0]);
    float mixedValue1, mixedValue2;
    mixedRng.GetNextGaussianPair(mixedValue1, mixedValue2);
    VERIFY_RANDOM(mixedValue1 == sequentialValues[1]);
    VERIFY_RANDOM(mixedValue2 == sequentialValues[2]);
    VERIFY_RANDOM(mixedRng.GetNextGaussian() == sequentialValues[3]);

    VERIFY_RANDOM(std::isfinite(BE1::RNG::GetGaussianRandom()));

    constexpr int pairCount = 65536;
    double sum = 0.0;
    double squaredSum = 0.0;
    BE1::RNG distributionRng(0x8f3f73b5cf1c9adeull);
    for (int i = 0; i < pairCount; i++) {
        float value0, value1;
        distributionRng.GetNextGaussianPair(value0, value1);
        sum += static_cast<double>(value0) + static_cast<double>(value1);
        squaredSum += static_cast<double>(value0) * value0 + static_cast<double>(value1) * value1;
    }

    const double sampleCount = pairCount * 2.0;
    const double mean = sum / sampleCount;
    const double variance = squaredSum / sampleCount - mean * mean;
    VERIFY_RANDOM(std::abs(mean) < 0.02);
    VERIFY_RANDOM(std::abs(variance - 1.0) < 0.02);
}

#undef VERIFY_RANDOM

#define VERIFY_FLOAT_TO_INTEGER(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestFloatToIntegerConversions() {
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();

    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi(1.9f) == 1);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi(-1.9f) == -1);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi(infinity) == std::numeric_limits<int>::max());
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi(-infinity) == std::numeric_limits<int>::min());
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi(nan) == std::numeric_limits<int>::min());

    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi8(126.9f) == 126);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi8(-127.9f) == -127);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi8(infinity) == 127);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi8(-infinity) == -128);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi8(nan) == -128);

    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi16(32766.9f) == 32766);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi16(-32767.9f) == -32767);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi16(infinity) == 32767);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi16(-infinity) == -32768);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoi16(nan) == -32768);

    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoui16(65534.9f) == 65534);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoui16(-1.0f) == 0);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoui16(infinity) == 65535);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftoui16(nan) == 0);

    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftob(254.9f) == 254);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftob(-1.0f) == 0);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftob(infinity) == 255);
    VERIFY_FLOAT_TO_INTEGER(BE1::Math::Ftob(nan) == 0);
}

#undef VERIFY_FLOAT_TO_INTEGER

#define VERIFY_EIGEN_ANALYSIS(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestEigenAnalysis() {
    const float epsilon = 1e-4f;
    BE1::Mat2 symmetric2(2.0f, 1.0f, 1.0f, 2.0f);
    BE1::Vec2 eigenValues2;
    BE1::Mat2 eigenVectors2;
    VERIFY_EIGEN_ANALYSIS(symmetric2.EigenDecomposeSymmetric(eigenValues2, eigenVectors2));
    VERIFY_EIGEN_ANALYSIS(eigenValues2.Equals(BE1::Vec2(3.0f, 1.0f), epsilon));

    for (int i = 0; i < 2; i++) {
        BE1::Vec2 eigenVector(eigenVectors2[0][i], eigenVectors2[1][i]);
        BE1::Vec2 residual = symmetric2 * eigenVector - eigenVector * eigenValues2[i];
        VERIFY_EIGEN_ANALYSIS(residual.Length() <= epsilon);
    }

    BE1::Mat2 diagonal2(1.0f, 0.0f, 0.0f, 5.0f);
    VERIFY_EIGEN_ANALYSIS(diagonal2.EigenDecomposeSymmetric(eigenValues2, eigenVectors2));
    VERIFY_EIGEN_ANALYSIS(eigenValues2.Equals(BE1::Vec2(5.0f, 1.0f), epsilon));

    for (int i = 0; i < 2; i++) {
        BE1::Vec2 eigenVector(eigenVectors2[0][i], eigenVectors2[1][i]);
        BE1::Vec2 residual = diagonal2 * eigenVector - eigenVector * eigenValues2[i];
        VERIFY_EIGEN_ANALYSIS(residual.Length() <= epsilon);
    }

    BE1::Vec2 unchangedValues2(7.0f, 8.0f);
    BE1::Mat2 unchangedVectors2 = BE1::Mat2::identity;
    BE1::Mat2 nonSymmetric2(1.0f, 2.0f, 0.0f, 1.0f);
    VERIFY_EIGEN_ANALYSIS(!nonSymmetric2.EigenDecomposeSymmetric(unchangedValues2, unchangedVectors2));
    VERIFY_EIGEN_ANALYSIS(unchangedValues2.Equals(BE1::Vec2(7.0f, 8.0f)));
    VERIFY_EIGEN_ANALYSIS(unchangedVectors2.Equals(BE1::Mat2::identity));

    BE1::Mat3 diagonal(
        BE1::Vec3(5.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 3.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 1.0f));

    BE1::Vec3 eigenValues;
    BE1::Mat3 eigenVectors;
    VERIFY_EIGEN_ANALYSIS(diagonal.EigenDecomposeSymmetric(eigenValues, eigenVectors));
    VERIFY_EIGEN_ANALYSIS(eigenValues.Equals(BE1::Vec3(5.0f, 3.0f, 1.0f), epsilon));

    for (int i = 0; i < 3; i++) {
        BE1::Vec3 residual = diagonal * eigenVectors[i] - eigenVectors[i] * eigenValues[i];
        VERIFY_EIGEN_ANALYSIS(residual.Length() <= epsilon);
    }

    BE1::Mat3 symmetric(
        BE1::Vec3(2.0f, 1.0f, 0.0f),
        BE1::Vec3(1.0f, 2.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 1.0f));
    VERIFY_EIGEN_ANALYSIS(symmetric.EigenDecomposeSymmetric(eigenValues, eigenVectors));
    VERIFY_EIGEN_ANALYSIS(eigenValues.Equals(BE1::Vec3(3.0f, 1.0f, 1.0f), epsilon));

    for (int i = 0; i < 3; i++) {
        BE1::Vec3 residual = symmetric * eigenVectors[i] - eigenVectors[i] * eigenValues[i];
        VERIFY_EIGEN_ANALYSIS(residual.Length() <= epsilon);
    }

    BE1::EigenPair3 powerResult;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver3::PowerIteration(diagonal, BE1::Vec3::one, powerResult, 32, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(powerResult.eigenValue - 5.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(powerResult.eigenVector.x) >= 1.0f - epsilon);

    BE1::EigenPair3 inversePowerResult;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver3::InversePowerIteration(
        diagonal, 0.0f, BE1::Vec3::one, inversePowerResult, 32, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult.eigenValue - 1.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult.eigenVector.z) >= 1.0f - epsilon);

    BE1::EigenPair3 rayleighResult;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver3::RayleighQuotientIteration(
        diagonal, BE1::Vec3(0.05f, 1.0f, 0.1f), rayleighResult, 8, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(rayleighResult.eigenValue - 3.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(rayleighResult.eigenVector.y) >= 1.0f - epsilon);

    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver3::RayleighQuotientIteration(
        diagonal, BE1::Vec3(1.0f, 0.0f, 0.0f), rayleighResult, 8, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(rayleighResult.numIterations == 0);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(rayleighResult.eigenValue - 5.0f) <= epsilon);

    BE1::Mat4 diagonal4(
        BE1::Vec4(7.0f, 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 4.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 2.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    BE1::EigenPair4 powerResult4;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver4::PowerIteration(diagonal4, BE1::Vec4::one, powerResult4, 48, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(powerResult4.eigenValue - 7.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(powerResult4.eigenVector.x) >= 1.0f - epsilon);

    BE1::EigenPair4 inversePowerResult4;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver4::InversePowerIteration(
        diagonal4, 0.0f, BE1::Vec4::one, inversePowerResult4, 32, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult4.eigenValue - 1.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult4.eigenVector.w) >= 1.0f - epsilon);

    const BE1::Mat4 pivoted4(
        BE1::Vec4(0.0f, 1.0f, 0.0f, 0.0f),
        BE1::Vec4(1.0f, 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 2.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 3.0f));
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver4::InversePowerIteration(
        pivoted4, 0.25f, BE1::Vec4::one, inversePowerResult4, 32, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult4.eigenValue - 1.0f) <= epsilon);

    BE1::EigenPair4 rayleighResult4;
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver4::RayleighQuotientIteration(
        diagonal4, BE1::Vec4(0.02f, 1.0f, 0.1f, 0.03f), rayleighResult4, 8, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(rayleighResult4.eigenValue - 4.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(rayleighResult4.eigenVector.y) >= 1.0f - epsilon);

    BE1::EigenPair4 unchangedResult4 = rayleighResult4;
    VERIFY_EIGEN_ANALYSIS(!BE1::EigenSolver4::RayleighQuotientIteration(
        BE1::Mat4::identity, BE1::Vec4::one, unchangedResult4, 0, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(unchangedResult4.eigenVector.Equals(rayleighResult4.eigenVector));

    BE1::Mat2 svdInput2(3.0f, 1.0f, 0.0f, 0.0f);
    BE1::Mat2 svdU2;
    BE1::Vec2 singularValues2;
    BE1::Mat2 svdV2;
    VERIFY_EIGEN_ANALYSIS(svdInput2.SingularValueDecompose(svdU2, singularValues2, svdV2));
    VERIFY_EIGEN_ANALYSIS(singularValues2[0] >= singularValues2[1]);
    VERIFY_EIGEN_ANALYSIS(singularValues2[1] == 0.0f);

    BE1::Mat2 singularMatrix2(
        singularValues2[0], 0.0f,
        0.0f, singularValues2[1]);
    BE1::Mat2 reconstructed2 = svdU2 * singularMatrix2;
    reconstructed2 *= svdV2.Transpose();
    VERIFY_EIGEN_ANALYSIS(reconstructed2.Equals(svdInput2, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdU2.Transpose() * svdU2).Equals(BE1::Mat2::identity, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdV2.Transpose() * svdV2).Equals(BE1::Mat2::identity, 5e-4f));

    BE1::Mat3 svdInput3(
        BE1::Vec3(3.0f, 0.0f, 0.0f),
        BE1::Vec3(1.0f, 2.0f, 0.0f),
        BE1::Vec3(0.0f, 1.0f, 0.0f));
    BE1::Mat3 svdU3;
    BE1::Vec3 singularValues3;
    BE1::Mat3 svdV3;
    VERIFY_EIGEN_ANALYSIS(svdInput3.SingularValueDecompose(svdU3, singularValues3, svdV3));
    VERIFY_EIGEN_ANALYSIS(singularValues3[0] >= singularValues3[1]);
    VERIFY_EIGEN_ANALYSIS(singularValues3[1] >= singularValues3[2]);
    VERIFY_EIGEN_ANALYSIS(singularValues3[2] == 0.0f);

    BE1::Mat3 singularMatrix3(
        BE1::Vec3(singularValues3[0], 0.0f, 0.0f),
        BE1::Vec3(0.0f, singularValues3[1], 0.0f),
        BE1::Vec3(0.0f, 0.0f, singularValues3[2]));
    BE1::Mat3 reconstructed3 = svdU3 * singularMatrix3;
    reconstructed3 *= svdV3.Transpose();
    VERIFY_EIGEN_ANALYSIS(reconstructed3.Equals(svdInput3, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdU3.Transpose() * svdU3).Equals(BE1::Mat3::identity, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdV3.Transpose() * svdV3).Equals(BE1::Mat3::identity, 5e-4f));

    BE1::Mat4 svdInput4(
        BE1::Vec4(4.0f, 1.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 3.0f, 1.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 2.0f, 1.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    BE1::Mat4 svdU4;
    BE1::Vec4 singularValues4;
    BE1::Mat4 svdV4;
    VERIFY_EIGEN_ANALYSIS(svdInput4.SingularValueDecompose(svdU4, singularValues4, svdV4));
    VERIFY_EIGEN_ANALYSIS(singularValues4[0] >= singularValues4[1]);
    VERIFY_EIGEN_ANALYSIS(singularValues4[1] >= singularValues4[2]);
    VERIFY_EIGEN_ANALYSIS(singularValues4[2] >= singularValues4[3]);
    VERIFY_EIGEN_ANALYSIS(singularValues4[3] == 0.0f);

    BE1::Mat4 singularMatrix4(
        BE1::Vec4(singularValues4[0], 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, singularValues4[1], 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, singularValues4[2], 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, singularValues4[3]));
    BE1::Mat4 reconstructed4 = svdU4 * singularMatrix4;
    reconstructed4 *= svdV4.Transpose();
    VERIFY_EIGEN_ANALYSIS(reconstructed4.Equals(svdInput4, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdU4.Transpose() * svdU4).Equals(BE1::Mat4::identity, 5e-4f));
    VERIFY_EIGEN_ANALYSIS((svdV4.Transpose() * svdV4).Equals(BE1::Mat4::identity, 5e-4f));

    // The shifted matrix requires a row pivot in its first column.
    BE1::Mat3 pivoted(
        BE1::Vec3(0.0f, 1.0f, 0.0f),
        BE1::Vec3(1.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 2.0f));
    VERIFY_EIGEN_ANALYSIS(BE1::EigenSolver3::InversePowerIteration(
        pivoted, 0.25f, BE1::Vec3(1.0f, 2.0f, 1.0f), inversePowerResult, 32, 1e-5f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(inversePowerResult.eigenValue - 1.0f) <= epsilon);
}

template <typename MatrixType>
static void VerifyMoorePenroseConditions(const MatrixType &matrix, const MatrixType &pseudoInverse, float epsilon) {
    MatrixType matrixProjection = matrix * pseudoInverse;
    MatrixType pseudoInverseProjection = pseudoInverse * matrix;

    VERIFY_EIGEN_ANALYSIS((matrixProjection * matrix).Equals(matrix, epsilon));
    VERIFY_EIGEN_ANALYSIS((pseudoInverseProjection * pseudoInverse).Equals(pseudoInverse, epsilon));
    VERIFY_EIGEN_ANALYSIS(matrixProjection.Equals(matrixProjection.Transpose(), epsilon));
    VERIFY_EIGEN_ANALYSIS(pseudoInverseProjection.Equals(pseudoInverseProjection.Transpose(), epsilon));
}

static void TestMatrixConditionAndPseudoInverse() {
    const float epsilon = 1e-3f;

    const BE1::Mat2 diagonal2(4.0f, 0.0f, 0.0f, 2.0f);
    float condition2 = -1.0f;
    VERIFY_EIGEN_ANALYSIS(diagonal2.ConditionNumber(condition2));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(condition2 - 2.0f) <= epsilon);

    BE1::Mat2 pseudoInverse2;
    VERIFY_EIGEN_ANALYSIS(diagonal2.PseudoInverse(pseudoInverse2));
    VERIFY_EIGEN_ANALYSIS(pseudoInverse2.Equals(BE1::Mat2(0.25f, 0.0f, 0.0f, 0.5f), epsilon));
    VerifyMoorePenroseConditions(diagonal2, pseudoInverse2, epsilon);

    const BE1::Mat2 rankOne2 = BE1::Mat2::FromOuterProduct(BE1::Vec2(1.0f, 2.0f), BE1::Vec2(3.0f, -1.0f));
    VERIFY_EIGEN_ANALYSIS(rankOne2.PseudoInverse(pseudoInverse2));
    VerifyMoorePenroseConditions(rankOne2, pseudoInverse2, epsilon);
    VERIFY_EIGEN_ANALYSIS(rankOne2.ConditionNumber(condition2));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::IsInf(condition2));

    BE1::Mat2 zeroPseudoInverse = BE1::Mat2::identity;
    VERIFY_EIGEN_ANALYSIS(BE1::Mat2::zero.PseudoInverse(zeroPseudoInverse));
    VERIFY_EIGEN_ANALYSIS(zeroPseudoInverse.Equals(BE1::Mat2::zero));
    VERIFY_EIGEN_ANALYSIS(BE1::Mat2::zero.ConditionNumber(condition2));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::IsInf(condition2));

    const BE1::Mat2 threshold2(1.0f, 0.0f, 0.0f, 1e-5f);
    VERIFY_EIGEN_ANALYSIS(threshold2.ConditionNumber(condition2, 1e-4f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::IsInf(condition2));
    VERIFY_EIGEN_ANALYSIS(threshold2.ConditionNumber(condition2, 1e-6f));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(condition2 - 1e5f) <= 20.0f);

    const BE1::Mat3 diagonal3(
        BE1::Vec3(6.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 3.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 2.0f));
    float condition3 = -1.0f;
    VERIFY_EIGEN_ANALYSIS(diagonal3.ConditionNumber(condition3));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(condition3 - 3.0f) <= epsilon);
    VERIFY_EIGEN_ANALYSIS((diagonal3 * 10000.0f).ConditionNumber(condition3));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(condition3 - 3.0f) <= epsilon);

    BE1::Mat3 pseudoInverse3;
    VERIFY_EIGEN_ANALYSIS(diagonal3.PseudoInverse(pseudoInverse3));
    const BE1::Mat3 expectedPseudoInverse3(
        BE1::Vec3(1.0f / 6.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 1.0f / 3.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 0.5f));
    VERIFY_EIGEN_ANALYSIS(pseudoInverse3.Equals(expectedPseudoInverse3, epsilon));
    VerifyMoorePenroseConditions(diagonal3, pseudoInverse3, epsilon);

    const BE1::Mat3 rankOne3 = BE1::Mat3::FromOuterProduct(
        BE1::Vec3(1.0f, 2.0f, -1.0f), BE1::Vec3(2.0f, -1.0f, 3.0f));
    VERIFY_EIGEN_ANALYSIS(rankOne3.PseudoInverse(pseudoInverse3));
    VerifyMoorePenroseConditions(rankOne3, pseudoInverse3, epsilon);
    VERIFY_EIGEN_ANALYSIS(rankOne3.ConditionNumber(condition3));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::IsInf(condition3));

    const BE1::Mat4 diagonal4(
        BE1::Vec4(8.0f, 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 4.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 2.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    float condition4 = -1.0f;
    VERIFY_EIGEN_ANALYSIS(diagonal4.ConditionNumber(condition4));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(condition4 - 8.0f) <= epsilon);

    BE1::Mat4 pseudoInverse4;
    VERIFY_EIGEN_ANALYSIS(diagonal4.PseudoInverse(pseudoInverse4));
    const BE1::Mat4 expectedPseudoInverse4(
        BE1::Vec4(0.125f, 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.25f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.5f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    VERIFY_EIGEN_ANALYSIS(pseudoInverse4.Equals(expectedPseudoInverse4, epsilon));
    VerifyMoorePenroseConditions(diagonal4, pseudoInverse4, epsilon);

    const BE1::Mat4 rankOne4 = BE1::Mat4::FromOuterProduct(
        BE1::Vec4(1.0f, 2.0f, -1.0f, 0.5f), BE1::Vec4(2.0f, -1.0f, 3.0f, 4.0f));
    VERIFY_EIGEN_ANALYSIS(rankOne4.PseudoInverse(pseudoInverse4));
    VerifyMoorePenroseConditions(rankOne4, pseudoInverse4, 2e-3f);
    VERIFY_EIGEN_ANALYSIS(rankOne4.ConditionNumber(condition4));
    VERIFY_EIGEN_ANALYSIS(BE1::Math::IsInf(condition4));

    float unchangedCondition = -7.0f;
    BE1::Mat2 unchangedPseudoInverse = BE1::Mat2::identity;
    VERIFY_EIGEN_ANALYSIS(!diagonal2.ConditionNumber(unchangedCondition, 0.0f));
    VERIFY_EIGEN_ANALYSIS(unchangedCondition == -7.0f);
    VERIFY_EIGEN_ANALYSIS(!diagonal2.PseudoInverse(unchangedPseudoInverse, 0.0f));
    VERIFY_EIGEN_ANALYSIS(unchangedPseudoInverse.Equals(BE1::Mat2::identity));

    const BE1::Mat2 nonFinite2(INFINITY, 0.0f, 0.0f, 1.0f);
    VERIFY_EIGEN_ANALYSIS(!nonFinite2.ConditionNumber(unchangedCondition));
    VERIFY_EIGEN_ANALYSIS(unchangedCondition == -7.0f);
    VERIFY_EIGEN_ANALYSIS(!nonFinite2.PseudoInverse(unchangedPseudoInverse));
    VERIFY_EIGEN_ANALYSIS(unchangedPseudoInverse.Equals(BE1::Mat2::identity));
}

static void TestSolveSymmetricPSD() {
    const float epsilon = 1e-3f;

    const BE1::Mat2 positiveDefinite2(4.0f, 1.0f, 1.0f, 3.0f);
    const BE1::Vec2 expected2(1.0f, 2.0f);
    const BE1::Vec2 rightHandSide2 = positiveDefinite2 * expected2;
    BE1::Vec2 solution2(7.0f, 8.0f);
    VERIFY_EIGEN_ANALYSIS(positiveDefinite2.SolveSymmetricPSD(rightHandSide2, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(expected2, epsilon));

    BE1::Vec2 aliasedSolution2 = rightHandSide2;
    VERIFY_EIGEN_ANALYSIS(positiveDefinite2.SolveSymmetricPSD(aliasedSolution2, aliasedSolution2));
    VERIFY_EIGEN_ANALYSIS(aliasedSolution2.Equals(expected2, epsilon));

    const BE1::Mat2 rankDeficient2(2.0f, 0.0f, 0.0f, 0.0f);
    VERIFY_EIGEN_ANALYSIS(rankDeficient2.SolveSymmetricPSD(BE1::Vec2(4.0f, 3.0f), solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(BE1::Vec2(2.0f, 0.0f), epsilon));

    solution2.Set(7.0f, 8.0f);
    VERIFY_EIGEN_ANALYSIS(BE1::Mat2::zero.SolveSymmetricPSD(BE1::Vec2(2.0f, -3.0f), solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(BE1::Vec2::zero));

    const BE1::Vec3 outerVector3(1.0f, -2.0f, 0.5f);
    const BE1::Mat3 positiveDefinite3 =
        BE1::Mat3::FromOuterProduct(outerVector3, outerVector3) + BE1::Mat3::identity * 2.0f;
    const BE1::Vec3 expected3(0.5f, -1.0f, 2.0f);
    const BE1::Vec3 rightHandSide3 = positiveDefinite3 * expected3;
    BE1::Vec3 solution3;
    VERIFY_EIGEN_ANALYSIS(positiveDefinite3.SolveSymmetricPSD(rightHandSide3, solution3));
    VERIFY_EIGEN_ANALYSIS(solution3.Equals(expected3, epsilon));

    const BE1::Mat3 rankDeficient3(
        BE1::Vec3(3.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 1.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 0.0f));
    VERIFY_EIGEN_ANALYSIS(rankDeficient3.SolveSymmetricPSD(BE1::Vec3(6.0f, -2.0f, 5.0f), solution3));
    VERIFY_EIGEN_ANALYSIS(solution3.Equals(BE1::Vec3(2.0f, -2.0f, 0.0f), epsilon));

    const BE1::Vec4 outerVector4(1.0f, 0.5f, -1.0f, 2.0f);
    const BE1::Mat4 positiveDefinite4 =
        BE1::Mat4::FromOuterProduct(outerVector4, outerVector4) + BE1::Mat4::identity * 1.5f;
    const BE1::Vec4 expected4(1.0f, -2.0f, 0.5f, 3.0f);
    const BE1::Vec4 rightHandSide4 = positiveDefinite4 * expected4;
    BE1::Vec4 solution4;
    VERIFY_EIGEN_ANALYSIS(positiveDefinite4.SolveSymmetricPSD(rightHandSide4, solution4));
    VERIFY_EIGEN_ANALYSIS(solution4.Equals(expected4, epsilon));

    const BE1::Mat4 rankDeficient4(
        BE1::Vec4(4.0f, 0.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 2.0f, 0.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 1.0f, 0.0f),
        BE1::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    VERIFY_EIGEN_ANALYSIS(rankDeficient4.SolveSymmetricPSD(BE1::Vec4(8.0f, -4.0f, 3.0f, 7.0f), solution4));
    VERIFY_EIGEN_ANALYSIS(solution4.Equals(BE1::Vec4(2.0f, -2.0f, 3.0f, 0.0f), epsilon));

    const BE1::Vec2 unchangedSolution2(7.0f, 8.0f);
    solution2 = unchangedSolution2;
    VERIFY_EIGEN_ANALYSIS(!BE1::Mat2(1.0f, 0.0f, 0.0f, -1.0f).SolveSymmetricPSD(BE1::Vec2::one, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));

    VERIFY_EIGEN_ANALYSIS(!BE1::Mat2(1.0f, 2.0f, 0.0f, 1.0f).SolveSymmetricPSD(BE1::Vec2::one, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(!positiveDefinite2.SolveSymmetricPSD(BE1::Vec2::one, solution2, 0.0f));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(!positiveDefinite2.SolveSymmetricPSD(BE1::Vec2(INFINITY, 0.0f), solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(!BE1::Mat2(INFINITY, 0.0f, 0.0f, 1.0f).SolveSymmetricPSD(BE1::Vec2::one, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
}

static void TestMatrixSolve() {
    const float epsilon = 1e-4f;

    // The leading zero requires a row interchange at the first pivot.
    const BE1::Mat2 pivoted2(0.0f, 2.0f, 1.0f, 3.0f);
    const BE1::Vec2 expected2(2.0f, -1.0f);
    const BE1::Vec2 rightHandSide2 = pivoted2 * expected2;
    BE1::Vec2 solution2(7.0f, 8.0f);
    VERIFY_EIGEN_ANALYSIS(pivoted2.Solve(rightHandSide2, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(expected2, epsilon));

    BE1::Vec2 aliasedSolution2 = rightHandSide2;
    VERIFY_EIGEN_ANALYSIS(pivoted2.Solve(aliasedSolution2, aliasedSolution2));
    VERIFY_EIGEN_ANALYSIS(aliasedSolution2.Equals(expected2, epsilon));

    // General Solve accepts nonsingular symmetric-indefinite matrices.
    const BE1::Mat2 indefinite2(1.0f, 0.0f, 0.0f, -2.0f);
    const BE1::Vec2 indefiniteExpected2(-3.0f, 2.0f);
    VERIFY_EIGEN_ANALYSIS(indefinite2.Solve(indefinite2 * indefiniteExpected2, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(indefiniteExpected2, epsilon));

    const BE1::Mat3 pivoted3(
        BE1::Vec3(0.0f, 1.0f, 2.0f),
        BE1::Vec3(2.0f, -1.0f, 1.0f),
        BE1::Vec3(1.0f, 0.0f, 3.0f));
    const BE1::Vec3 expected3(1.0f, -2.0f, 0.5f);
    BE1::Vec3 solution3;
    VERIFY_EIGEN_ANALYSIS(pivoted3.Solve(pivoted3 * expected3, solution3));
    VERIFY_EIGEN_ANALYSIS(solution3.Equals(expected3, epsilon));

    const BE1::Mat4 pivoted4(
        BE1::Vec4(0.0f, 1.0f, 0.0f, 0.0f),
        BE1::Vec4(1.0f, 2.0f, 1.0f, 0.0f),
        BE1::Vec4(0.0f, 1.0f, 3.0f, 1.0f),
        BE1::Vec4(0.0f, 0.0f, 1.0f, 4.0f));
    const BE1::Vec4 expected4(1.0f, -2.0f, 0.5f, 3.0f);
    BE1::Vec4 solution4;
    VERIFY_EIGEN_ANALYSIS(pivoted4.Solve(pivoted4 * expected4, solution4));
    VERIFY_EIGEN_ANALYSIS(solution4.Equals(expected4, epsilon));

    const BE1::Mat2 smallScale2 = pivoted2 * 1e-20f;
    VERIFY_EIGEN_ANALYSIS(smallScale2.Solve(smallScale2 * expected2, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(expected2, epsilon));
    const BE1::Mat2 largeScale2 = pivoted2 * 1e20f;
    VERIFY_EIGEN_ANALYSIS(largeScale2.Solve(largeScale2 * expected2, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(expected2, epsilon));

    const BE1::Vec2 unchangedSolution2(7.0f, 8.0f);
    solution2 = unchangedSolution2;
    const BE1::Mat2 singular2(1.0f, 2.0f, 2.0f, 4.0f);
    VERIFY_EIGEN_ANALYSIS(!singular2.Solve(BE1::Vec2::one, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));

    const BE1::Mat2 nearSingular2(1.0f, 0.0f, 0.0f, 1e-7f);
    const BE1::Vec2 nearSingularExpected2(2.0f, 3.0f);
    const BE1::Vec2 nearSingularRightHandSide2 = nearSingular2 * nearSingularExpected2;
    VERIFY_EIGEN_ANALYSIS(!nearSingular2.Solve(nearSingularRightHandSide2, solution2, 1e-6f));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(nearSingular2.Solve(nearSingularRightHandSide2, solution2, 1e-8f));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(nearSingularExpected2, epsilon));

    solution2 = unchangedSolution2;
    VERIFY_EIGEN_ANALYSIS(!pivoted2.Solve(rightHandSide2, solution2, 0.0f));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(!pivoted2.Solve(BE1::Vec2(INFINITY, 0.0f), solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
    VERIFY_EIGEN_ANALYSIS(!BE1::Mat2(INFINITY, 0.0f, 0.0f, 1.0f).Solve(BE1::Vec2::one, solution2));
    VERIFY_EIGEN_ANALYSIS(solution2.Equals(unchangedSolution2));
}

static void TestCovarianceAndPrincipalComponents() {
    const float epsilon = 1e-4f;
    BE1::Vec2 eigenValues2;
    BE1::Mat2 eigenVectors2;

    BE1::CovarianceAccumulator3 covarianceA;
    covarianceA.AddPoint(BE1::Vec3(-1.0f, 0.0f, 0.0f));

    BE1::CovarianceAccumulator3 covarianceB;
    covarianceB.AddPoint(BE1::Vec3(1.0f, 0.0f, 0.0f));
    covarianceA.Merge(covarianceB);

    VERIFY_EIGEN_ANALYSIS(covarianceA.Count() == 2);
    VERIFY_EIGEN_ANALYSIS(covarianceA.Mean().Equals(BE1::Vec3::zero, epsilon));

    BE1::Mat3 covariance = covarianceA.PopulationCovariance();
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(covariance[0][0] - 1.0f) <= epsilon);

    BE1::CovarianceAccumulator2 covariance2A;
    covariance2A.AddPoint(BE1::Vec2(-1.0f, -2.0f));

    BE1::CovarianceAccumulator2 covariance2B;
    covariance2B.AddPoint(BE1::Vec2(1.0f, 2.0f));
    covariance2A.Merge(covariance2B);

    VERIFY_EIGEN_ANALYSIS(covariance2A.Count() == 2);
    VERIFY_EIGEN_ANALYSIS(covariance2A.Mean().Equals(BE1::Vec2::zero, epsilon));
    BE1::Mat2 covariance2 = covariance2A.PopulationCovariance();
    VERIFY_EIGEN_ANALYSIS(covariance2.Equals(BE1::Mat2(1.0f, 2.0f, 2.0f, 4.0f), epsilon));
    BE1::Mat2 sampleCovariance2 = covariance2A.SampleCovariance();
    VERIFY_EIGEN_ANALYSIS(sampleCovariance2.Equals(BE1::Mat2(2.0f, 4.0f, 4.0f, 8.0f), epsilon));
    VERIFY_EIGEN_ANALYSIS(covariance2.EigenDecomposeSymmetric(eigenValues2, eigenVectors2));
    VERIFY_EIGEN_ANALYSIS(eigenValues2.Equals(BE1::Vec2(5.0f, 0.0f), epsilon));
    for (int i = 0; i < 2; i++) {
        BE1::Vec2 eigenVector(eigenVectors2[0][i], eigenVectors2[1][i]);
        BE1::Vec2 residual = covariance2 * eigenVector - eigenVector * eigenValues2[i];
        VERIFY_EIGEN_ANALYSIS(residual.Length() <= epsilon);
    }
    BE1::PrincipalComponents2 principalComponents2;
    VERIFY_EIGEN_ANALYSIS(principalComponents2.SetFromCovariance(covariance2));
    VERIFY_EIGEN_ANALYSIS(principalComponents2.eigenValues.Equals(BE1::Vec2(5.0f, 0.0f), epsilon));
    VERIFY_EIGEN_ANALYSIS(principalComponents2.rank == 1);
    BE1::Vec2 principalAxis2(principalComponents2.axes[0][0], principalComponents2.axes[1][0]);
    BE1::Vec2 expectedPrincipalAxis2(1.0f, 2.0f);
    expectedPrincipalAxis2.Normalize();
    VERIFY_EIGEN_ANALYSIS(principalAxis2.Equals(expectedPrincipalAxis2, epsilon));

    BE1::PrincipalComponents2 isotropicComponents2;
    VERIFY_EIGEN_ANALYSIS(isotropicComponents2.SetFromCovariance(BE1::Mat2(2.0f, 0.0f, 0.0f, 2.0f)));
    VERIFY_EIGEN_ANALYSIS(isotropicComponents2.eigenValues.Equals(BE1::Vec2(2.0f, 2.0f), epsilon));
    VERIFY_EIGEN_ANALYSIS(isotropicComponents2.axes.Equals(BE1::Mat2::identity));
    VERIFY_EIGEN_ANALYSIS(isotropicComponents2.rank == 2);

    BE1::PrincipalComponents2 zeroComponents2;
    VERIFY_EIGEN_ANALYSIS(zeroComponents2.SetFromCovariance(BE1::Mat2::zero));
    VERIFY_EIGEN_ANALYSIS(zeroComponents2.eigenValues.Equals(BE1::Vec2::zero));
    VERIFY_EIGEN_ANALYSIS(zeroComponents2.axes.Equals(BE1::Mat2::identity));
    VERIFY_EIGEN_ANALYSIS(zeroComponents2.rank == 0);

    BE1::PrincipalComponents2 unchangedComponents2 = principalComponents2;
    VERIFY_EIGEN_ANALYSIS(!unchangedComponents2.SetFromCovariance(BE1::Mat2(1.0f, 0.0f, 0.0f, -1.0f)));
    VERIFY_EIGEN_ANALYSIS(unchangedComponents2.eigenValues.Equals(principalComponents2.eigenValues));
    VERIFY_EIGEN_ANALYSIS(unchangedComponents2.axes.Equals(principalComponents2.axes));
    VERIFY_EIGEN_ANALYSIS(unchangedComponents2.rank == principalComponents2.rank);

    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(covariance[1][1]) <= epsilon);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(covariance[2][2]) <= epsilon);

    BE1::PrincipalComponents3 principalComponents;
    VERIFY_EIGEN_ANALYSIS(principalComponents.SetFromCovariance(covariance));
    VERIFY_EIGEN_ANALYSIS(principalComponents.rank == 1);
    VERIFY_EIGEN_ANALYSIS(BE1::Math::Fabs(principalComponents.axes[0].x) >= 1.0f - epsilon);
}

static void TestMatrixDecomposition() {
    const float epsilon = 1e-4f;
    BE1::Vec3 eigenValues;
    BE1::Mat3 eigenVectors;

    const BE1::Mat2 asymmetric2(1.0f, 2.0f, 4.0f, 5.0f);
    const BE1::Mat2 expectedSymmetricPart2(1.0f, 3.0f, 3.0f, 5.0f);
    const BE1::Mat2 expectedSkewSymmetricPart2(0.0f, -1.0f, 1.0f, 0.0f);
    VERIFY_EIGEN_ANALYSIS(asymmetric2.Symmetrize().Equals(expectedSymmetricPart2));
    VERIFY_EIGEN_ANALYSIS(asymmetric2.SkewSymmetrize().Equals(expectedSkewSymmetricPart2));
    VERIFY_EIGEN_ANALYSIS((expectedSymmetricPart2 + expectedSkewSymmetricPart2).Equals(asymmetric2));
    VERIFY_EIGEN_ANALYSIS(asymmetric2[0][1] == 2.0f);
    BE1::Mat2 symmetricPart2 = asymmetric2;
    BE1::Mat2 skewSymmetricPart2 = asymmetric2;
    VERIFY_EIGEN_ANALYSIS(symmetricPart2.SymmetrizeSelf().Equals(expectedSymmetricPart2));
    VERIFY_EIGEN_ANALYSIS(skewSymmetricPart2.SkewSymmetrizeSelf().Equals(expectedSkewSymmetricPart2));

    const BE1::Mat3 asymmetric(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    const BE1::Mat3 expectedSymmetricPart(
        1.0f, 3.0f, 5.0f,
        3.0f, 5.0f, 7.0f,
        5.0f, 7.0f, 9.0f);
    VERIFY_EIGEN_ANALYSIS(asymmetric.Symmetrize().Equals(expectedSymmetricPart));
    VERIFY_EIGEN_ANALYSIS(asymmetric[0][1] == 2.0f);
    BE1::Mat3 symmetricPart = asymmetric;
    VERIFY_EIGEN_ANALYSIS(symmetricPart.SymmetrizeSelf().Equals(expectedSymmetricPart));

    const BE1::Mat3 expectedSkewSymmetricPart(
        0.0f, -1.0f, -2.0f,
        1.0f, 0.0f, -1.0f,
        2.0f, 1.0f, 0.0f);
    VERIFY_EIGEN_ANALYSIS(asymmetric.SkewSymmetrize().Equals(expectedSkewSymmetricPart));
    VERIFY_EIGEN_ANALYSIS(asymmetric[0][0] == 1.0f);
    BE1::Mat3 skewSymmetricPart = asymmetric;
    VERIFY_EIGEN_ANALYSIS(skewSymmetricPart.SkewSymmetrizeSelf().Equals(expectedSkewSymmetricPart));

    const BE1::Mat4 asymmetric4(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f);
    const BE1::Mat4 expectedSymmetricPart4(
        1.0f, 3.5f, 6.0f, 8.5f,
        3.5f, 6.0f, 8.5f, 11.0f,
        6.0f, 8.5f, 11.0f, 13.5f,
        8.5f, 11.0f, 13.5f, 16.0f);
    const BE1::Mat4 expectedSkewSymmetricPart4(
        0.0f, -1.5f, -3.0f, -4.5f,
        1.5f, 0.0f, -1.5f, -3.0f,
        3.0f, 1.5f, 0.0f, -1.5f,
        4.5f, 3.0f, 1.5f, 0.0f);
    VERIFY_EIGEN_ANALYSIS(asymmetric4.Symmetrize().Equals(expectedSymmetricPart4));
    VERIFY_EIGEN_ANALYSIS(asymmetric4.SkewSymmetrize().Equals(expectedSkewSymmetricPart4));
    VERIFY_EIGEN_ANALYSIS((expectedSymmetricPart4 + expectedSkewSymmetricPart4).Equals(asymmetric4));
    VERIFY_EIGEN_ANALYSIS(asymmetric4[0][1] == 2.0f);
    BE1::Mat4 symmetricPart4 = asymmetric4;
    BE1::Mat4 skewSymmetricPart4 = asymmetric4;
    VERIFY_EIGEN_ANALYSIS(symmetricPart4.SymmetrizeSelf().Equals(expectedSymmetricPart4));
    VERIFY_EIGEN_ANALYSIS(skewSymmetricPart4.SkewSymmetrizeSelf().Equals(expectedSkewSymmetricPart4));

    const BE1::Mat3 expectedOrthogonal = BE1::Mat3::FromRotationZYX(
        DEG2RAD(35.0f), DEG2RAD(-20.0f), DEG2RAD(15.0f));
    const BE1::Mat3 expectedSymmetric(
        2.0f, 0.25f, -0.1f,
        0.25f, 1.5f, 0.2f,
        -0.1f, 0.2f, 0.75f);
    const BE1::Mat3 polarInput = expectedOrthogonal * expectedSymmetric;

    BE1::Mat3 orthogonalFactor;
    BE1::Mat3 symmetricFactor;
    VERIFY_EIGEN_ANALYSIS(polarInput.PolarDecompose(orthogonalFactor, symmetricFactor));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.Equals(expectedOrthogonal, 2e-4f));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.Equals(expectedSymmetric, 2e-4f));
    VERIFY_EIGEN_ANALYSIS((orthogonalFactor * symmetricFactor).Equals(polarInput, 2e-4f));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.IsOrthogonal(2e-4f));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.IsSymmetric(2e-4f));

    const BE1::Mat3 reflection(
        BE1::Vec3(-1.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 1.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 1.0f));
    VERIFY_EIGEN_ANALYSIS(reflection.IsOrthogonal());
    const BE1::Mat3 reflectedInput = reflection * expectedSymmetric;
    VERIFY_EIGEN_ANALYSIS(reflectedInput.PolarDecompose(orthogonalFactor, symmetricFactor));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.Equals(reflection, 2e-4f));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.Determinant() < 0.0f);
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.Equals(expectedSymmetric, 2e-4f));

    const BE1::Mat3 singularStretch(
        BE1::Vec3(3.0f, 0.0f, 0.0f),
        BE1::Vec3(0.0f, 1.0f, 0.0f),
        BE1::Vec3(0.0f, 0.0f, 0.0f));
    const BE1::Mat3 singularPolarInput = expectedOrthogonal * singularStretch;
    VERIFY_EIGEN_ANALYSIS(singularPolarInput.PolarDecompose(orthogonalFactor, symmetricFactor));
    VERIFY_EIGEN_ANALYSIS((orthogonalFactor * symmetricFactor).Equals(singularPolarInput, 5e-4f));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.IsOrthogonal(5e-4f));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.IsSymmetric(5e-4f));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.EigenDecomposeSymmetric(eigenValues, eigenVectors));
    VERIFY_EIGEN_ANALYSIS(eigenValues.z >= -epsilon);

    VERIFY_EIGEN_ANALYSIS(BE1::Mat3::zero.PolarDecompose(orthogonalFactor, symmetricFactor));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.Equals(BE1::Mat3::identity));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.Equals(BE1::Mat3::zero));

    orthogonalFactor = expectedOrthogonal;
    symmetricFactor = expectedSymmetric;
    VERIFY_EIGEN_ANALYSIS(!polarInput.PolarDecompose(orthogonalFactor, symmetricFactor, 0.0f));
    VERIFY_EIGEN_ANALYSIS(orthogonalFactor.Equals(expectedOrthogonal));
    VERIFY_EIGEN_ANALYSIS(symmetricFactor.Equals(expectedSymmetric));
}

#undef VERIFY_EIGEN_ANALYSIS

#define VERIFY_BEST_FIT(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestBestFit() {
    const float epsilon = 1e-4f;

    BE1::Vec3 expectedLineDirection(2.0f, -1.0f, 0.5f);
    expectedLineDirection.Normalize();
    const BE1::Vec3 lineCenter(3.0f, -4.0f, 2.0f);
    const BE1::Vec3 linePoints[] = {
        lineCenter - expectedLineDirection * 3.0f,
        lineCenter - expectedLineDirection,
        lineCenter + expectedLineDirection,
        lineCenter + expectedLineDirection * 3.0f
    };

    BE1::Line line;
    VERIFY_BEST_FIT(BE1::BestFitLine(linePoints, 4, line));
    VERIFY_BEST_FIT(line.pos.Equals(lineCenter, epsilon));
    VERIFY_BEST_FIT(line.dir.Equals(expectedLineDirection, epsilon));

    const BE1::Vec3 isotropicPoints[] = {
        BE1::Vec3::unitX, -BE1::Vec3::unitX,
        BE1::Vec3::unitY, -BE1::Vec3::unitY,
        BE1::Vec3::unitZ, -BE1::Vec3::unitZ
    };
    const BE1::Line unchangedLine(BE1::Vec3(7.0f, 8.0f, 9.0f), BE1::Vec3::unitY);
    BE1::Line ambiguousLine = unchangedLine;
    VERIFY_BEST_FIT(!BE1::BestFitLine(isotropicPoints, 6, ambiguousLine));
    VERIFY_BEST_FIT(ambiguousLine.Equals(unchangedLine));

    BE1::Vec3 expectedPlaneNormal(1.0f, 2.0f, -1.0f);
    expectedPlaneNormal.Normalize();
    BE1::Vec3 planeAxis0(2.0f, -1.0f, 0.0f);
    planeAxis0.Normalize();
    BE1::Vec3 planeAxis1 = expectedPlaneNormal.Cross(planeAxis0);
    planeAxis1.Normalize();
    const BE1::Vec3 planeCenter(-2.0f, 1.0f, 5.0f);
    const BE1::Vec3 planePoints[] = {
        planeCenter + planeAxis0 * 2.0f,
        planeCenter - planeAxis0 * 2.0f,
        planeCenter + planeAxis0,
        planeCenter - planeAxis0,
        planeCenter + planeAxis1 * 3.0f,
        planeCenter - planeAxis1 * 3.0f
    };

    BE1::Plane plane;
    VERIFY_BEST_FIT(BE1::BestFitPlane(planePoints, 6, plane));
    VERIFY_BEST_FIT(BE1::Math::Fabs(plane.normal.Dot(expectedPlaneNormal)) >= 1.0f - epsilon);
    for (int i = 0; i < 6; i++) {
        VERIFY_BEST_FIT(BE1::Math::Fabs(plane.Distance(planePoints[i])) <= epsilon);
    }

    const BE1::Vec3 collinearPoints[] = {
        BE1::Vec3(-1.0f, 0.0f, 0.0f),
        BE1::Vec3::zero,
        BE1::Vec3(1.0f, 0.0f, 0.0f)
    };
    const BE1::Plane unchangedPlane(BE1::Vec3::unitZ, 4.0f);
    BE1::Plane ambiguousPlane = unchangedPlane;
    VERIFY_BEST_FIT(!BE1::BestFitPlane(collinearPoints, 3, ambiguousPlane));
    VERIFY_BEST_FIT(ambiguousPlane.normal.Equals(unchangedPlane.normal));
    VERIFY_BEST_FIT(ambiguousPlane.offset == unchangedPlane.offset);

    const BE1::Vec3 sourcePoints[] = {
        BE1::Vec3(-2.0f, -1.0f, 0.0f),
        BE1::Vec3(3.0f, -1.0f, 1.0f),
        BE1::Vec3(0.0f, 2.0f, -1.0f),
        BE1::Vec3(1.0f, 1.0f, 3.0f),
        BE1::Vec3(-1.0f, 0.0f, -2.0f),
        BE1::Vec3(2.0f, 3.0f, 1.0f)
    };
    const BE1::Mat3 expectedRotation = BE1::Mat3::FromRotationZYX(DEG2RAD(37.0f), DEG2RAD(-21.0f), DEG2RAD(13.0f));
    const BE1::Vec3 expectedTranslation(4.0f, -3.0f, 2.0f);
    BE1::Vec3 targetPoints[6];
    for (int i = 0; i < 6; i++) {
        targetPoints[i] = expectedRotation * sourcePoints[i] + expectedTranslation;
    }

    BE1::Mat3x4 rigidTransform;
    VERIFY_BEST_FIT(BE1::BestFitRigidTransform(sourcePoints, targetPoints, 6, rigidTransform));
    VERIFY_BEST_FIT(rigidTransform.ToMat3().Equals(expectedRotation, 5e-4f));
    VERIFY_BEST_FIT(rigidTransform.ToTranslationVec3().Equals(expectedTranslation, 5e-4f));
    for (int i = 0; i < 6; i++) {
        VERIFY_BEST_FIT(rigidTransform.TransformPos(sourcePoints[i]).Equals(targetPoints[i], 1e-3f));
    }

    BE1::Vec3 reflectedPoints[6];
    for (int i = 0; i < 6; i++) {
        reflectedPoints[i].Set(-sourcePoints[i].x, sourcePoints[i].y, sourcePoints[i].z);
    }
    BE1::Mat3x4 reflectionRejectedTransform;
    VERIFY_BEST_FIT(BE1::BestFitRigidTransform(sourcePoints, reflectedPoints, 6, reflectionRejectedTransform));
    VERIFY_BEST_FIT(reflectionRejectedTransform.ToMat3().Determinant() > 0.0f);

    const BE1::Vec3 degenerateTargetPoints[] = {
        BE1::Vec3(2.0f, 1.0f, 0.0f),
        BE1::Vec3(3.0f, 1.0f, 0.0f),
        BE1::Vec3(4.0f, 1.0f, 0.0f)
    };
    const BE1::Mat3x4 unchangedTransform(BE1::Mat3::identity, BE1::Vec3(8.0f, 9.0f, 10.0f));
    BE1::Mat3x4 degenerateTransform = unchangedTransform;
    VERIFY_BEST_FIT(!BE1::BestFitRigidTransform(collinearPoints, degenerateTargetPoints, 3, degenerateTransform));
    VERIFY_BEST_FIT(degenerateTransform.Equals(unchangedTransform));
}

#undef VERIFY_BEST_FIT

#define VERIFY_ROTATION_CONVERSION(expression) VerifyMathTest((expression), __func__, __LINE__)

static void TestRotationConversions() {
    BE1::Mat3 m = BE1::Mat3::FromRotationZYX(DEG2RAD(120), DEG2RAD(-20), DEG2RAD(60));

    BE1::Rotation r = m.ToRotation();
    BE1::Mat3 m2 = r.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m2, 0.0001f));

    BE1::Quat q = m.ToQuat();
    BE1::Mat3 m3 = q.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m3, 0.0001f));

    BE1::Angles a = m.ToAngles();
    BE1::Mat3 m4 = a.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m4, 0.0001f));

    BE1::Rotation r2 = q.ToRotation();
    BE1::Mat3 m5 = r2.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m5, 0.0001f));

    BE1::Angles a2 = q.ToAngles();
    BE1::Mat3 m6 = a2.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m6, 0.0001f));
    VERIFY_ROTATION_CONVERSION(a.Equals(a2, 0.0001f));

    BE1::Quat q2 = a.ToQuat();
    BE1::Mat3 m7 = q2.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m7, 0.0001f));
    VERIFY_ROTATION_CONVERSION(q.Equals(q2, 0.0001f));

    BE1::Angles a3 = r.ToAngles();
    BE1::Mat3 m8 = a3.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m8, 0.0001f));
    VERIFY_ROTATION_CONVERSION(a.Equals(a3, 0.0001f));

    BE1::Quat q3 = r.ToQuat();
    BE1::Mat3 m9 = q3.ToMat3();
    VERIFY_ROTATION_CONVERSION(m.Equals(m9, 0.0001f));
    VERIFY_ROTATION_CONVERSION(q.Equals(q3, 0.0001f));
}

#undef VERIFY_ROTATION_CONVERSION

void Test::TestMath() {
    TestSquareRootContract();
    TestPolynomialSolvers();
    TestSphereIntersections();
    TestIntegerLog2();
    TestRandom();
    TestFloatToIntegerConversions();
    TestEigenAnalysis();
    TestMatrixConditionAndPseudoInverse();
    TestSolveSymmetricPSD();
    TestMatrixSolve();
    TestCovarianceAndPrincipalComponents();
    TestBestFit();
    TestMatrixDecomposition();
    TestRotationConversions();

    BenchmarkSqrt();
    BenchmarkInvSqrt();
    BenchmarkTrigonometry();
    BenchmarkLog();
    BenchmarkExp();
    BenchmarkPow();
}
