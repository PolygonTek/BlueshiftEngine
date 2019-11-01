shader "Image" {
    properties {
        albedoMap("Albedo Map") : texture "2D" = "_whiteTexture"
        intensity("Intensity") : float range 0 16.0 0.001 = "1"
    }

    generatePerforatedVersion

    glsl_vp {
        #define _ALBEDO 1
        $include "Unlit.vp"
    }
    glsl_fp {
        #define _ALBEDO 1
        $include "Unlit.fp"
    }
}
