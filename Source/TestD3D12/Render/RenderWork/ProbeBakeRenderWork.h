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

#include "Precompiled.h"
#include "Render/ProbeBakeJob.h"
#include "RenderWork.h"

class ProbeBakeRenderWork : public RenderWork {
public:
    ProbeBakeRenderWork(uint64_t bakeSerial, const ProbeBakeJob::Desc &jobDesc, bool generateRadianceMipChain);

    virtual const char *            GetDebugName() const override { return "ProbeBakeRenderWork"; }
    virtual bool                    RecordCommands(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) override;
    virtual void                    OnCompleted(bool submitted) override;

private:
    uint64_t                        bakeSerial = 0;
    ProbeBakeJob                    bakeJob;
    bool                            generateRadianceMipChain = false;
};
