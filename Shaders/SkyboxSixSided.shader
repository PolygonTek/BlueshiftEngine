shader "Skybox/SixSided" {
    skySurface
    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyBackMap("Back (+X)") : texture "2D" = "_defaultTexture"
        skyFrontMap("Front (-X)") : texture "2D" = "_defaultTexture"
        skyRightMap("Right (+Y))") : texture "2D" = "_defaultTexture"
        skyLeftMap("Left (-Y)") : texture "2D" = "_defaultTexture"
        skyTopMap("Top (+Z)") : texture "2D" = "_defaultTexture"
        skyDownMap("Down (-Z)") : texture "2D" = "_defaultTexture"
    }

    glsl_vp {
        $include "SkyboxSixSided.vp"
    }

    glsl_fp {
        $include "SkyboxSixSided.fp"
    }
}
