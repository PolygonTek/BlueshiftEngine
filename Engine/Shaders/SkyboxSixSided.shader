shader "Skybox/SixSided" {
    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyBackMap("Back (+X)") : object TextureAsset = "_defaultTexture"
        skyFrontMap("Front (-X)") : object TextureAsset = "_defaultTexture"
        skyRightMap("Right (+Y))") : object TextureAsset = "_defaultTexture"
        skyLeftMap("Left (-Y)") : object TextureAsset = "_defaultTexture"
        skyTopMap("Top (+Z)") : object TextureAsset = "_defaultTexture"
        skyDownMap("Down (-Z)") : object TextureAsset = "_defaultTexture"
    }

    glsl_vp {
        $include "SkyboxSixSided.vp"
    }

    glsl_fp {
        $include "SkyboxSixSided.fp"
    }
}
