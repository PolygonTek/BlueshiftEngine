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
#include "RenderWorkQueue.h"
#include "Render/RenderFrameData.h"

void RenderWorkQueue::Submit(UniquePtr<RenderWork> work) {
    assert(BE1::Engine::IsInMainThread());

    if (!work) {
        return;
    }

    pendingWorks.Append(std::move(work));
}

int RenderWorkQueue::AttachPendingWorks(RenderFrameData *frameData, int maxWorkCount) {
    assert(BE1::Engine::IsInMainThread());

    if (!frameData || maxWorkCount <= 0) {
        return 0;
    }

    int attachedWorkCount = 0;

    while (!pendingWorks.IsEmpty() && attachedWorkCount < maxWorkCount) {
        UniquePtr<RenderWork> work = pendingWorks.TakeFirst();

        if (!frameData->CmdRenderWork(work.get())) {
            // 렌더 커맨드에 밀어 넣지 못했다면, 다시 대기 목록으로 되돌린다.
            pendingWorks.Insert(std::move(work), 0);
            break;
        }

        AttachedWork attachedWork;
        attachedWork.renderWork = std::move(work);
        attachedWork.frameData = frameData;
        attachedWorks.Append(std::move(attachedWork));
        attachedWorkCount++;
    }

    return attachedWorkCount;
}

void RenderWorkQueue::CompleteAttachedWork(AttachedWork &attachedWork) {
    if (!attachedWork.renderWork) {
        return;
    }

    const bool wasSubmitted = attachedWork.renderWork->WasSubmitted();

    attachedWork.renderWork->OnCompleted(wasSubmitted);
    attachedWork.renderWork.reset();
    attachedWork.frameData = nullptr;
}

void RenderWorkQueue::CompleteAttachedWorks(const RenderFrameData *completedFrameDataArray, int completedFrameDataCount) {
    assert(BE1::Engine::IsInMainThread());

    if (!completedFrameDataArray || completedFrameDataCount <= 0) {
        return;
    }

    for (int index = 0; index < attachedWorks.Count();) {
        AttachedWork &attachedWork = attachedWorks[index];

        bool belongsToCompletedFrame = false;
        for (int completedFrameDataIndex = 0; completedFrameDataIndex < completedFrameDataCount; ++completedFrameDataIndex) {
            if (attachedWork.frameData == &completedFrameDataArray[completedFrameDataIndex]) {
                belongsToCompletedFrame = true;
                break;
            }
        }

        if (!belongsToCompletedFrame) {
            index++;
            continue;
        }

        CompleteAttachedWork(attachedWork);
        attachedWorks.RemoveIndex(index);
    }
}

void RenderWorkQueue::CompleteAttachedWorks(const RenderFrameData *completedFrameData) {
    CompleteAttachedWorks(completedFrameData, 1);
}

void RenderWorkQueue::CompleteAllAttachedWorks() {
    assert(BE1::Engine::IsInMainThread());

    for (AttachedWork &attachedWork : attachedWorks) {
        CompleteAttachedWork(attachedWork);
    }
    attachedWorks.SetCount(0, false);
}

void RenderWorkQueue::CancelPendingWorks() {
    assert(BE1::Engine::IsInMainThread());

    for (UniquePtr<RenderWork> &work : pendingWorks) {
        if (work) {
            work->OnCompleted(false);
            work.reset();
        }
    }
    pendingWorks.SetCount(0, false);
}
