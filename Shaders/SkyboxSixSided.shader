shader "Skybox/SixSided" {
    skySurface
    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyBackMap("Back (+X)") : texture = "_defaultTexture"
        skyFrontMap("Front (-X)") : texture = "_defaultTexture"
        skyRightMap("Right (+Y))") : texture = "_defaultTexture"
        skyLeftMap("Left (-Y)") : texture = "_defaultTexture"
        skyTopMap("Top (+Z)") : texture = "_defaultTexture"
        skyDownMap("Down (-Z)") : texture = "_defaultTexture"
    }

    glsl_vp {
        $include "SkyboxSixSided.vp"
    }

    glsl_fp {
        $include "SkyboxSixSided.fp"
    }
}
