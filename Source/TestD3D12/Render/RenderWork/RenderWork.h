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

namespace RHI {
    class CommandList;
    class FrameThreadData;
}

class RenderBackend;
class RenderWorkQueue;

/*
---------------------------------------------------------------------------------------------------

    Render Work

    RenderFrame 의 렌더 커맨드에 밀어 넣어져 렌더 백엔드에서 처리되는 비정기 렌더 작업이다.
    별도의 FrameThreadData 를 소유하지 않고 밀어 넣은 프레임의 0 번 FrameThreadData 를 사용한다.
    작업 객체는 일회성이며 RenderWorkQueue 가 OnCompleted 호출까지 수명을 관리한다.

---------------------------------------------------------------------------------------------------
*/

class RenderWork {
    friend class RenderBackend;
    friend class RenderWorkQueue;

public:
    virtual ~RenderWork() = default;

    virtual const char *            GetDebugName() const = 0;

                                    // 메인 스레드에서 frame command 에 추가할 준비가 되었는지 검사한다.
                                    // false 를 반환한 작업은 RenderWorkQueue 의 대기 목록에 유지된다.
    virtual bool                    IsReadyToAttach() const { return true; }

                                    // 렌더 백엔드에서 커맨드 리스트를 기록한다.
                                    // true 를 반환하면 백엔드가 커맨드 리스트를 닫아 그래픽스 큐에 제출하고,
                                    // false 를 반환하면 제출하지 않고 닫기만 한다.
    virtual bool                    RecordCommands(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) = 0;

                                    // 메인 스레드에서 한 번 호출된다.
                                    // 렌더 커맨드에 밀어 넣은 작업은 해당 프레임의 GPU 완료 후 호출되고,
                                    // 대기 중 취소된 작업은 즉시 OnCompleted(false) 로 호출된다.
    virtual void                    OnCompleted(bool commandListSubmitted) = 0;

private:
    void                            MarkCommandListSubmitted() { commandListSubmitted = true; }
    bool                            WasCommandListSubmitted() const { return commandListSubmitted; }

    bool                            commandListSubmitted = false;
    uint64_t                        enqueueTimeUsec = 0;
};
