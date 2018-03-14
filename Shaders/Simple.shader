shader "Simple" {
    properties {
        albedoMap("Albedo Map") : texture = "_whiteTexture"
    }

    glsl_vp {
        $include "Simple.vp"
    }
    glsl_fp {
        $include "Simple.fp"
    }
}
