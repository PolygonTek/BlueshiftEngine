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
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "SceneDebugDraw.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void SceneDebugDraw::Init(RenderWorld *renderWorld) {
    assert(!this->renderWorld);
    assert(renderWorld);

    this->renderWorld = renderWorld;
    renderWorld->ClearDebugPrimitives();
    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);
}

void SceneDebugDraw::Shutdown() {
    if (!renderWorld) {
        return;
    }

    renderWorld->ClearDebugPrimitives();
    renderWorld->SetDebugColor(BE1::Color4::zero, BE1::Color4::zero);

    renderWorld = nullptr;
}

void SceneDebugDraw::SetDrawColor(const BE1::Color4 &color) {
    const BE1::Color4 lineColor = drawLines ? color : BE1::Color4::zero;
    const BE1::Color4 fillColor = drawFill ? BE1::Color4(color.ToColor3(), 0.18f) : BE1::Color4::zero;
    renderWorld->SetDebugColor(lineColor, fillColor);
}

void SceneDebugDraw::DrawPlanarPrimitives(const BE1::Mat3 &axis) {
    const float z = 1.0f;

    SetDrawColor(BE1::Color4::white);
    renderWorld->DrawDebugPoint(BE1::Vec3(-36.0f, -20.0f, z), 0.1f, depthTest);

    const BE1::Vec3 lineCenter(-28.0f, -20.0f, z);
    SetDrawColor(BE1::Color4::red);
    renderWorld->DrawDebugLine(lineCenter - axis[0] * 3.0f, lineCenter + axis[0] * 3.0f, depthTest);

    const BE1::Vec3 triangleCenter(-20.0f, -20.0f, z);
    SetDrawColor(BE1::Color4::orange);
    renderWorld->DrawDebugTriangle(triangleCenter - axis[0] * 2.5f - axis[1] * 2.0f, triangleCenter + axis[0] * 2.5f - axis[1] * 2.0f, triangleCenter + axis[1] * 2.5f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::yellow);
    renderWorld->DrawDebugQuad(BE1::Vec3(-12.0f, -20.0f, z), axis[0], axis[1], 2.5f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::lime);
    renderWorld->DrawDebugCircle(BE1::Vec3(-4.0f, -20.0f, z), axis[2], 2.75f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::cyan);
    renderWorld->DrawDebugHollowCircle(BE1::Vec3(4.0f, -20.0f, z), axis[2], 1.5f, 2.75f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::cornflowerBlue);
    renderWorld->DrawDebugArc(BE1::Vec3(12.0f, -20.0f, z), axis[0], axis[1], 2.75f, 20.0f, 300.0f, false, twoSided, depthTest);

    SetDrawColor(BE1::Color4::blueViolet);
    renderWorld->DrawDebugArc(BE1::Vec3(20.0f, -20.0f, z), axis[0], axis[1], 2.75f, 20.0f, 300.0f, true, twoSided, depthTest);

    SetDrawColor(BE1::Color4::magenta);
    renderWorld->DrawDebugEllipse(BE1::Vec3(28.0f, -20.0f, z), axis[0], axis[1], 3.5f, 2.0f, twoSided, depthTest);

    const BE1::Vec3 polylinePoints[] = {
        BE1::Vec3(-12.0f, -30.0f, z),
        BE1::Vec3(-6.0f, -27.0f, z),
        BE1::Vec3(0.0f, -30.0f, z),
        BE1::Vec3(6.0f, -27.0f, z),
        BE1::Vec3(12.0f, -30.0f, z)
    };
    SetDrawColor(BE1::Color4::white);
    renderWorld->DrawDebugPolyline(polylinePoints, COUNT_OF(polylinePoints), false, depthTest);

    SetDrawColor(BE1::Color4::aqua);
    renderWorld->DrawDebugPlane(BE1::Vec3(26.0f, -30.0f, z), axis[2], 3.0f, depthTest);
}

void SceneDebugDraw::DrawBoundsPrimitives(const BE1::Mat3 &axis) {
    const BE1::Vec3 aabbCenter(-18.0f, -7.0f, 3.0f);
    const BE1::Vec3 aabbExtents(3.0f, 2.0f, 2.5f);
    SetDrawColor(BE1::Color4::orangeRed);
    renderWorld->DrawDebugAABB(BE1::AABB(aabbCenter - aabbExtents, aabbCenter + aabbExtents), twoSided, depthTest);

    SetDrawColor(BE1::Color4::greenYellow);
    renderWorld->DrawDebugOBB(BE1::OBB(BE1::Vec3(-6.0f, -7.0f, 3.0f), BE1::Vec3(3.0f, 2.0f, 2.5f), axis), twoSided, depthTest);

    BE1::Frustum frustum;
    frustum.SetOrigin(BE1::Vec3(8.0f, -8.0f, 1.0f));
    frustum.SetAxis(axis);
    frustum.SetSize(1.0f, 8.0f, 3.5f, 2.5f);
    SetDrawColor(BE1::Color4::aqua);
    renderWorld->DrawDebugFrustum(frustum, false, twoSided, depthTest);

    SetDrawColor(BE1::Color4::white);
    renderWorld->DrawDebugAxis(BE1::Vec3(26.0f, -7.0f, 1.0f), axis, 5.0f, depthTest);
}

void SceneDebugDraw::DrawRoundPrimitives(const BE1::Mat3 &axis) {
    const float z = 3.0f;

    SetDrawColor(BE1::Color4::coral);
    renderWorld->DrawDebugHemisphere(BE1::Vec3(-18.0f, 5.0f, z), axis, 3.0f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::gold);
    renderWorld->DrawDebugHemisphereSimple(BE1::Vec3(-6.0f, 5.0f, z), axis, 3.0f, depthTest);

    SetDrawColor(BE1::Color4::limeGreen);
    renderWorld->DrawDebugSphere(BE1::Vec3(6.0f, 5.0f, z), axis, 3.0f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::deepSkyBlue);
    renderWorld->DrawDebugSphereSimple(BE1::Vec3(18.0f, 5.0f, z), axis, 3.0f, depthTest);
}

void SceneDebugDraw::DrawAxialPrimitives(const BE1::Mat3 &axis) {
    const float z = 0.0f;
    const float height = 6.0f;
    const float rowY = 17.0f;
    const float firstX = -36.0f;
    const float spacing = 12.0f;

    SetDrawColor(BE1::Color4::tomato);
    renderWorld->DrawDebugCone(BE1::Vec3(firstX + spacing * 0.0f, rowY, z), axis, height, 0.0f, 2.5f, true, twoSided, depthTest);

    SetDrawColor(BE1::Color4::yellowGreen);
    renderWorld->DrawDebugConeSimple(BE1::Vec3(firstX + spacing * 1.0f, rowY, z), axis, height, 0.75f, 2.5f, depthTest);

    SetDrawColor(BE1::Color4::springGreen);
    renderWorld->DrawDebugCylinder(BE1::Vec3(firstX + spacing * 2.0f, rowY, z + 3.0f), axis, height, 2.25f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::dodgerBlue);
    renderWorld->DrawDebugCylinderSimple(BE1::Vec3(firstX + spacing * 3.0f, rowY, z + 3.0f), axis, height, 2.25f, depthTest);

    SetDrawColor(BE1::Color4::blueViolet);
    renderWorld->DrawDebugCapsule(BE1::Vec3(firstX + spacing * 4.0f, rowY, z + 3.0f), axis, 5.0f, 2.0f, twoSided, depthTest);

    SetDrawColor(BE1::Color4::deepPink);
    renderWorld->DrawDebugCapsuleSimple(BE1::Vec3(firstX + spacing * 5.0f, rowY, z + 3.0f), axis, 5.0f, 2.0f, depthTest);

    const BE1::Vec3 arrowCenter(firstX + spacing * 6.0f, rowY, z + 3.0f);
    SetDrawColor(BE1::Color4::white);
    renderWorld->DrawDebugArrow(arrowCenter - axis[2] * 3.0f, arrowCenter + axis[2] * 3.0f, 2.0f, 0.5f, depthTest);
}

void SceneDebugDraw::DrawTimedPrimitive(const BE1::Mat3 &axis) {
    renderWorld->SetDebugColor(BE1::Color4::magenta, BE1::Color4(BE1::Color4::magenta.ToColor3(), 0.25f));
    renderWorld->DrawDebugSphere(BE1::Vec3(0.0f, 40.0f, 5.0f), axis, 3.5f, twoSided, depthTest, 3000);
}

void SceneDebugDraw::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneDebugDraw::Update");
    assert(renderWorld);

    if (pendingClearDebugPrimitives) {
        pendingClearDebugPrimitives = false;
        renderWorld->ClearDebugPrimitives();
    }

    const float elapsedSeconds = static_cast<float>(MICRO2SEC(static_cast<double>(elapsedUsec)));
    const float phase = animate ? elapsedSeconds : 0.0f;
    const BE1::Mat3 axis = animate ? BE1::Mat3::FromRotationZ(phase * 0.5f) : BE1::Mat3::identity;

    if (drawLines || drawFill) {
        DrawPlanarPrimitives(axis);
        DrawBoundsPrimitives(axis);
        DrawRoundPrimitives(axis);
        DrawAxialPrimitives(axis);
    }

    if (pendingSpawnTimedPrimitive) {
        pendingSpawnTimedPrimitive = false;
        DrawTimedPrimitive(axis);
    }

    renderWorld->SetDebugColor(BE1::Color4::zero, BE1::Color4::zero);
}

#ifdef ENABLE_IMGUI
void SceneDebugDraw::BuildImGui() {
    ImGui::TextUnformatted("Debug Draw regression scene");
    ImGui::Checkbox("Animate##DebugDraw", &animate);
    ImGui::Checkbox("Lines##DebugDraw", &drawLines);
    ImGui::Checkbox("Fill##DebugDraw", &drawFill);
    ImGui::Checkbox("Depth test##DebugDraw", &depthTest);
    ImGui::Checkbox("Two sided##DebugDraw", &twoSided);

    ImGui::Separator();
    if (ImGui::Button("Spawn 3 second sphere##DebugDraw")) {
        pendingSpawnTimedPrimitive = true;
    }
    if (ImGui::Button("Clear debug primitives##DebugDraw")) {
        pendingClearDebugPrimitives = true;
    }
}
#endif
