shader "Unlit" {
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        albedoColor("Albedo Color") : color3 = "1 1 1"
        albedoAlpha("Albedo Alpha") : float range 0 1.0 0.001 = "1"
        albedoMap("Albedo Map") : texture = "_whiteTexture"
    }

    generatePerforatedVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion

    glsl_vp {
        $include "Unlit.vp"
    }
    glsl_fp {
        $include "Unlit.fp"
    }
}
