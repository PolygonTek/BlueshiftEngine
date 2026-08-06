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

#include "Scene/Scene.h"

class SceneDebugDraw : public Scene {
public:
    void                        Init(RenderWorld *renderWorld) override;
    void                        Shutdown() override;

    void                        Update(uint64_t elapsedUsec) override;

#ifdef ENABLE_IMGUI
    void                        BuildImGui() override;
#endif

private:
    void                        SetDrawColor(const BE1::Color4 &color);
    void                        DrawPlanarPrimitives(const BE1::Mat3 &axis);
    void                        DrawBoundsPrimitives(const BE1::Mat3 &axis);
    void                        DrawRoundPrimitives(const BE1::Mat3 &axis);
    void                        DrawAxialPrimitives(const BE1::Mat3 &axis);
    void                        DrawTimedPrimitive(const BE1::Mat3 &axis);

    RenderWorld *               renderWorld = nullptr;

    bool                        drawLines = true;
    bool                        drawFill = true;
    bool                        depthTest = true;
    bool                        twoSided = true;
    bool                        animate = true;

    bool                        pendingSpawnTimedPrimitive = false;
    bool                        pendingClearDebugPrimitives = false;
};
