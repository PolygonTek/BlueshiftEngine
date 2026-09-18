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
#include "ShaderPipelineBootstrap.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"

void ShaderPipelineBootstrap::BeginCollection(int expectedPipelineCount) {
    pipelines.Clear();
    if (expectedPipelineCount > 0) {
        pipelines.Reserve(expectedPipelineCount);
    }

    readyPipelineCount = 0;
    totalPipelineCount = 0;
    state = State::Collecting;
}

void ShaderPipelineBootstrap::TrackPipeline(const char *groupName, const SharedPtr<ShaderPipeline> &pipeline) {
    assert(state == State::Collecting);

    if (pipeline) {
        for (const TrackedPipeline &trackedPipeline : pipelines) {
            if (trackedPipeline.pipeline.get() == pipeline.get()) {
                return;
            }
        }
    }

    TrackedPipeline &trackedPipeline = pipelines.Alloc();
    trackedPipeline.groupName = groupName;
    trackedPipeline.pipeline = pipeline;
}

void ShaderPipelineBootstrap::EndCollection() {
    assert(state == State::Collecting);

    totalPipelineCount = pipelines.Count();
    state = pipelines.IsEmpty() ? State::Ready : State::Loading;
}

void ShaderPipelineBootstrap::Update() {
    if (state != State::Loading) {
        return;
    }

    int newReadyPipelineCount = 0;
    const TrackedPipeline *failedPipeline = nullptr;

    for (const TrackedPipeline &trackedPipeline : pipelines) {
        if (!trackedPipeline.pipeline) {
            failedPipeline = &trackedPipeline;
            break;
        }

        switch (trackedPipeline.pipeline->GetState()) {
        case ShaderPipeline::State::Ready:
            newReadyPipelineCount++;
            break;
        case ShaderPipeline::State::Building:
        case ShaderPipeline::State::Rebuilding:
            break;
        case ShaderPipeline::State::Empty:
        case ShaderPipeline::State::Failed:
            failedPipeline = &trackedPipeline;
            break;
        }

        if (failedPipeline) {
            break;
        }
    }

    readyPipelineCount = newReadyPipelineCount;

    if (failedPipeline) {
        BE_ERRLOG("ShaderPipelineBootstrap::Update: pipeline build failed (%s)\n", failedPipeline->groupName ? failedPipeline->groupName : "unknown");
        pipelines.Clear();
        state = State::Failed;
        return;
    }

    if (readyPipelineCount == totalPipelineCount) {
        pipelines.Clear();
        state = State::Ready;
    }
}

void ShaderPipelineBootstrap::Reset() {
    pipelines.Clear();
    readyPipelineCount = 0;
    totalPipelineCount = 0;
    state = State::Collecting;
}

float ShaderPipelineBootstrap::GetProgress() const {
    if (state == State::Ready) {
        return 1.0f;
    }
    if (totalPipelineCount <= 0) {
        return 0.0f;
    }
    return static_cast<float>(readyPipelineCount) / static_cast<float>(totalPipelineCount);
}
