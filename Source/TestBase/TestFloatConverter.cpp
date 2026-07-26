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

#include <cmath>
#include <limits>

static void TestF16Converter() {
    // +0과 -0 은 값은 같지만 F16 부호 비트가 각각 0 과 1 로 보존되어야 한다.
    assert(BE1::F16Converter::FromF32(0.0f) == 0x0000u);
    assert(BE1::F16Converter::FromF32(-0.0f) == 0x8000u);

    // 기본 FlushToZero 모드에서는 F16 denormal 범위의 값을 부호를 유지한 0 으로 저장한다.
    assert(BE1::F16Converter::FromF32(0x1p-15f) == 0x0000u);
    assert(BE1::F16Converter::FromF32(-0x1p-15f) == 0x8000u);

    // 이미 저장된 F16 최소 denormal 값은 ToF32 에서 정확한 float 값으로 복원해야 한다.
    assert(BE1::F16Converter::ToF32(0x0001u) == 0x1p-24f);

    // Preserve 모드에서는 F16 최소 denormal 값과 그 부호를 그대로 생성한다.
    assert(BE1::F16Converter::FromF32(0x1p-24f, BE1::DenormalMode::Preserve) == 0x0001u);
    assert(BE1::F16Converter::FromF32(-0x1p-24f, BE1::DenormalMode::Preserve) == 0x8001u);

    // F16 denormal 범위의 중간값도 올바른 가수 비트로 양자화해야 한다.
    assert(BE1::F16Converter::FromF32(0x1p-15f, BE1::DenormalMode::Preserve) == 0x0200u);

    // 최소 normal 값과 최대 denormal 값의 정확한 중간값은 짝수 쪽인 최소 normal 값으로 반올림한다.
    assert(BE1::F16Converter::FromF32(0x1p-14f - 0x1p-25f, BE1::DenormalMode::Preserve) == 0x0400u);

    // F16 가수의 정확한 중간값은 round-to-nearest-even 규칙으로 반올림해야 한다.
    assert(BE1::F16Converter::FromF32(1.0f + 0x1p-11f) == 0x3C00u);
    assert(BE1::F16Converter::FromF32(1.0f + 3.0f * 0x1p-11f) == 0x3C02u);

    // 지수 경계에서도 정확한 중간값은 짝수 쪽인 다음 지수의 값으로 반올림해야 한다.
    assert(BE1::F16Converter::FromF32(2.0f - 0x1p-11f) == 0x4000u);

    // F16 최대 유한값은 유지하고, 그 다음 반올림 경계부터는 부호를 보존한 INF 로 변환해야 한다.
    assert(BE1::F16Converter::FromF32(65504.0f) == 0x7BFFu);
    assert(BE1::F16Converter::FromF32(65520.0f) == 0x7C00u);
    assert(BE1::F16Converter::FromF32(-65520.0f) == 0xFC00u);

    // F16 INF 비트 패턴은 ToF32 에서도 무한대로 복원해야 한다.
    assert(std::isinf(BE1::F16Converter::ToF32(0x7C00u)));
}

static void TestUnsignedFloatConverters() {
    const float positiveInfinity = std::numeric_limits<float>::infinity();
    const float quietNaN = std::numeric_limits<float>::quiet_NaN();

    // F11 은 부호 비트가 없는 5비트 지수, 6 비트 가수 형식으로 1.0 과 +INF 를 표현한다.
    assert(BE1::F11Converter::FromF32(1.0f) == 0x3C0u);
    assert(BE1::F11Converter::FromF32(positiveInfinity) == 0x7C0u);

    // 부호를 표현할 수 없는 F11 에서는 음수 유한값과 -INF 를 0 으로 clamp 한다.
    assert(BE1::F11Converter::FromF32(-1.0f) == 0);
    assert(BE1::F11Converter::FromF32(-positiveInfinity) == 0);

    // F11 INF 비트 패턴은 ToF32 에서 무한대로 복원해야 한다.
    assert(std::isinf(BE1::F11Converter::ToF32(0x7C0u)));

    // Preserve 모드에서는 F11 최소 denormal 값을 생성해야 한다.
    assert(BE1::F11Converter::FromF32(0x1p-20f, BE1::DenormalMode::Preserve) == 0x001u);

    // F11 NaN 은 모든 지수 비트가 1 이고 가수가 0 이 아닌 형식으로 유지해야 한다.
    const uint32_t f11NaN = BE1::F11Converter::FromF32(quietNaN);
    assert((f11NaN & 0x7C0u) == 0x7C0u);
    assert((f11NaN & 0x03Fu) != 0);

    // F10 은 부호 비트가 없는 5 비트 지수, 5 비트 가수 형식으로 1.0 과 +INF 를 표현한다.
    assert(BE1::F10Converter::FromF32(1.0f) == 0x1E0u);
    assert(BE1::F10Converter::FromF32(positiveInfinity) == 0x3E0u);

    // 부호를 표현할 수 없는 F10 에서는 음수 유한값과 -INF 를 0 으로 clamp 한다.
    assert(BE1::F10Converter::FromF32(-1.0f) == 0);
    assert(BE1::F10Converter::FromF32(-positiveInfinity) == 0);

    // F10 INF 비트 패턴은 ToF32 에서 무한대로 복원해야 한다.
    assert(std::isinf(BE1::F10Converter::ToF32(0x3E0u)));

    // Preserve 모드에서는 F10 최소 denormal 값을 생성해야 한다.
    assert(BE1::F10Converter::FromF32(0x1p-19f, BE1::DenormalMode::Preserve) == 0x001u);

    // F10 NaN 은 모든 지수 비트가 1 이고 가수가 0 이 아닌 형식으로 유지해야 한다.
    const uint32_t f10NaN = BE1::F10Converter::FromF32(quietNaN);
    assert((f10NaN & 0x3E0u) == 0x3E0u);
    assert((f10NaN & 0x01Fu) != 0);
}

void Test::TestFloatConverter() {
    TestF16Converter();
    TestUnsignedFloatConverters();
}
