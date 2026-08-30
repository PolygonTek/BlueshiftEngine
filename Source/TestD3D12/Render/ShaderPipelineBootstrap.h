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

class ShaderPipeline;

/*
---------------------------------------------------------------------------------------------------

    Shader Pipeline Bootstrap

    - 초기 렌더링에 필요한 ShaderPipeline 집합의 빌드 상태와 진행률을 추적한다.
    - bootstrap 이 완료되거나 실패하면 추적용 strong reference 를 해제한다.
    - 런타임 shader rebuild 및 지연 pipeline 생성은 추적하지 않는다.

---------------------------------------------------------------------------------------------------
*/

class ShaderPipelineBootstrap {
public:
    enum class State : uint8_t {
        Collecting,                 /// BeginCollection 이후 ShaderPipeline 을 등록하는 상태
        Loading,                    /// 등록을 마치고 비동기 빌드 완료를 기다리는 상태
        Ready,                      /// 등록된 모든 ShaderPipeline 의 빌드가 완료된 상태
        Failed                      /// 하나 이상의 ShaderPipeline 빌드가 실패한 상태
    };

                                    /// Tracking 목록을 초기화하고 ShaderPipeline 등록을 시작한다.
    void                            BeginCollection(int expectedPipelineCount = 0);

                                    /// 초기 렌더링에 필요한 ShaderPipeline 하나를 tracking 목록에 등록한다.
    void                            TrackPipeline(const char *groupName, const SharedPtr<ShaderPipeline> &pipeline);

                                    /// 등록을 종료하고 ShaderPipeline 빌드 완료 대기를 시작한다.
    void                            EndCollection();

                                    /// 등록된 ShaderPipeline 들의 빌드 상태와 진행률을 갱신한다.
    void                            Update();

                                    /// Tracking 상태와 보관 중인 ShaderPipeline 참조를 초기화한다.
    void                            Reset();

    State                           GetState() const { return state; }
    bool                            IsReady() const { return state == State::Ready; }
    bool                            HasFailed() const { return state == State::Failed; }

                                    /// 전체 ShaderPipeline 중 빌드가 완료된 비율을 반환한다.
    float                           GetProgress() const;

    int                             GetReadyPipelineCount() const { return readyPipelineCount; }
    int                             GetTotalPipelineCount() const { return totalPipelineCount; }

private:
    struct TrackedPipeline {
        const char *                groupName = nullptr;
        SharedPtr<ShaderPipeline>   pipeline;
    };

    BE1::Array<TrackedPipeline>     pipelines;
    State                           state = State::Collecting;
    int                             readyPipelineCount = 0;
    int                             totalPipelineCount = 0;
};
