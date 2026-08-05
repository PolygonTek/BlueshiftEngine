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

#include "RenderWork.h"

class RenderFrameData;

/*
---------------------------------------------------------------------------------------------------

    Render Work Queue

---------------------------------------------------------------------------------------------------
*/

class RenderWorkQueue {
public:
                                    // render work 를 대기 목록에 추가한다.
    void                            Enqueue(UniquePtr<RenderWork> work);

                                    // 대기 중인 render work 를 최대 maxWorkCount 만큼 frameData 의 렌더 커맨드에 밀어 넣는다.
    int                             AttachPendingWorks(RenderFrameData *frameData, int maxWorkCount);

                                    // 완료된 frame data 의 렌더 커맨드에 밀어 넣었던 render work 를 완료 처리한다.
    void                            CompleteAttachedWorks(const RenderFrameData *completedFrameDataArray, int completedFrameDataCount);
    void                            CompleteAttachedWorks(const RenderFrameData *completedFrameData);

                                    // 렌더 커맨드에 밀어 넣은 모든 render work 를 완료 처리한다.
    void                            CompleteAllAttachedWorks();

                                    // 대기 중인 모든 render work 를 취소한다.
    void                            CancelPendingWorks();

private:
    struct AttachedWork {
        UniquePtr<RenderWork>       renderWork;
        const RenderFrameData *     frameData = nullptr;
    };

    void                            CompleteAttachedWork(AttachedWork &attachedWork);

    BE1::Array<UniquePtr<RenderWork>> pendingWorks;     ///< 렌더 커맨드에 밀어 넣기를 기다리는 render works
    BE1::Array<AttachedWork>        attachedWorks;      ///< 렌더 커맨드에 밀어 넣어져 완료를 기다리는 render works
};
