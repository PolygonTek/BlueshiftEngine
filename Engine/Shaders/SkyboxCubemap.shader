shader "Skybox/CubeMap" {
    skySurface
    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyCubeMap("Cube Map") : texture = "_defaultCubeTexture"
    }

    glsl_vp {
        $include "SkyboxCubemap.vp"
    }

    glsl_fp {
        $include "SkyboxCubemap.fp"
    }
}
