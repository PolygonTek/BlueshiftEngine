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

#include "Containers/Array.h"

// 카메라별 요청 해상도만 보관한다.
class ShadowResolutionHistory {
public:
    void                            BeginView();
    void                            Reset();

                                    // 광원의 이전 기록으로 요청 해상도를 결정하고 현재 뷰의 기록을 갱신한다.
    uint32_t                        Update(uint32_t lightIndex, uint64_t lightId, uint32_t maxResolution, float scaledProjectedSize);

                                    // 배율을 적용한 화면 투영 크기와 이전 해상도를 기준으로 히스테리시스를 적용해 요청 해상도를 선택한다.
    static uint32_t                 SelectResolution(uint32_t previousResolution, uint32_t maxResolution, float scaledProjectedSize);

private:
    struct Entry {
        uint64_t                    lightId = 0;
        uint32_t                    viewIndex = 0;
        uint32_t                    resolution = 0;
    };

    BE1::Array<Entry>               entries;
    uint32_t                        viewIndex = 0;
};
