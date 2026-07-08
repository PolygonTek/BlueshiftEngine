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

BE_NAMESPACE_BEGIN

static thread_local RNG rng(time(nullptr));

int RNG::GetRandom(int minValue, int maxValue) {
    return rng.GetNextInt(minValue, maxValue);
}

int RNG::GetRandom(int maxValue) {
    return RNG::GetRandom(0, maxValue);
}

uint32_t RNG::GetRandom(uint32_t minValue, uint32_t maxValue) {
    return rng.GetNextUInt(minValue, maxValue);
}

uint32_t RNG::GetRandom(uint32_t maxValue) {
    return RNG::GetRandom(0u, maxValue);
}

uint64_t RNG::GetRandom(uint64_t minValue, uint64_t maxValue) {
    return rng.GetNextUInt(minValue, maxValue);
}

uint64_t RNG::GetRandom(uint64_t maxValue) {
    return RNG::GetRandom(0ull, maxValue);
}

float RNG::GetRandom(float minValue, float maxValue) {
    return rng.GetNextFloat(minValue, maxValue);
}

float RNG::GetRandom(float maxValue) {
    return RNG::GetRandom(0.0f, maxValue);
}

BE_NAMESPACE_END
