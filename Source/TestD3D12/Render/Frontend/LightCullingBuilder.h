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
#include "Math/Rect.h"

class RenderFrame;
struct FrameCamera;

// 카메라별 광원 목록으로 CPU light culling 데이터를 구성한다.
// 작업 상태를 소유하지 않으며 결과는 전달받은 프레임 데이터에 기록한다.
class LightCullingBuilder {
public:
    LightCullingBuilder() = delete;

                                    // 출력 영역 좌표의 clip rect 를 타일 범위로 변환하고, 렌더 픽셀 기준 긴 변의 길이를 반환한다.
    static uint32_t                 ComputeTileRange(const FrameCamera *frameCamera, const BE1::Rect &screenClipRect, uint16_t &packedTileMin, uint16_t &packedTileMax);

                                    // 수집된 광원 목록을 제자리에서 정렬하고 Z-bin 및 프레임 공용 테이블 범위를 구성한다.
                                    // lightCulling 이 기본 초기화된 현재 뷰에 대해 한 번 호출한다.
    static void                     Build(RenderFrame *frame, FrameCamera *frameCamera, float zNear, float zFar);

private:
    static void                     BuildZBins(RenderFrame *frame, FrameCamera *frameCamera, float zNear, float zFar);
};
