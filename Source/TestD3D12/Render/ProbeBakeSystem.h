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

#include "ProbeBakeJob.h"

class RenderContext;
class RenderWorld;
class ProbeBakeRenderWork;

/*
---------------------------------------------------------------------------------------------------

    Probe Bake System

    프로브 베이크 요청을 스케줄링한다. 디스플레이 프레임 전에는 필요한 radiance cubemap 을
    캡처하고, mip 생성과 필터링은 RenderWorkQueue 에 제출한다. RenderWorld 객체는 지속적인
    프로브 상태와 생성된 cubemap 을 보관한다.

---------------------------------------------------------------------------------------------------
*/

class ProbeBakeSystem {
    friend class ProbeBakeRenderWork;

public:
    void                            Init();
    void                            Shutdown();

    void                            ScheduleEnvironmentLightProbeBake(RenderWorld *renderWorld, const BE1::Vec3 &captureOrigin);
    void                            CancelEnvironmentLightProbeBake(RenderWorld *renderWorld);

    void                            Prepare(RenderContext *renderContext);

private:
    struct EnvironmentLightBakeRequest {
        RenderWorld *               renderWorld = nullptr;
        BE1::Vec3                   captureOrigin = BE1::Vec3::origin;
    };

    void                            CompleteBake(uint64_t bakeSerial, bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap);
    void                            ApplyEnvironmentLightBakeResult(RenderWorld *renderWorld, bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap);

    BE1::Array<EnvironmentLightBakeRequest> environmentLightRequests;
    EnvironmentLightBakeRequest     activeEnvironmentLightRequest;
    uint64_t                        nextBakeSerial = 0;
    uint64_t                        activeBakeSerial = 0;
};
