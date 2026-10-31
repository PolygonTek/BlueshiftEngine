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

#include "Render/Ocean/GpuOcean.h"
#include "Render/Ocean/GpuOceanSurface.h"

class RenderWorld;

class OceanSurface {
    friend class RenderWorld;

public:
    struct Desc {
        GpuOcean::SimulationSettings  simulation;
        GpuOceanSurface::Settings     surface;

        bool                          IsValid() const { return simulation.IsValid() && surface.IsValid(); }
    };

    const Desc &                      GetDesc() const { return desc; }

private:
    void                              Update(const Desc &desc);

    Desc                              desc;
    uint64_t                          revision = 0;
    BE1::Array<OceanSurfaceQueryInput> queries;
};
