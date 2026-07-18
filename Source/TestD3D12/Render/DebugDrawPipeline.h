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

#pragma once

#include <cstdint>

enum class DebugDrawPipelineId : uint8_t {
    DepthTestedOneSidedFill,    // 깊이 테스트 On + one sided 면
    DepthTestedTwoSidedFill,    // 깊이 테스트 On + two sided 면
    DepthTestedLines,           // 깊이 테스트 On + 라인
    OneSidedFill,               // 깊이 테스트 Off + one sided 면
    TwoSidedFill,               // 깊이 테스트 Off + two sided 면
    Lines,                      // 깊이 테스트 Off + 라인
    Count
};
