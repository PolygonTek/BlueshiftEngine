#pragma once

#include "Render/Camera.h"

class SceneManager;

enum class ScenePreset {
    Default,
    Builtin,
    Skull,
    Killeroo,
    LegoCar,
    CornellBox,
    Sponza,
    Sibenik,
    Optimus
};

struct SceneView {
    PinholeCamera                   camera;
    float                           unitMeter = 1.0f;
};

void BuildScene(SceneManager &scene, SceneView &view, int width, int height, ScenePreset preset = ScenePreset::Default);
