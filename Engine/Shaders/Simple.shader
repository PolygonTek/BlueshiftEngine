shader "Simple" {
    properties {
        albedoMap("Albedo Map") : object TextureAsset = "_whiteTexture"
    }

    glsl_vp {
        $include "Simple.vp"
    }
    glsl_fp {
        $include "Simple.fp"
    }
}
