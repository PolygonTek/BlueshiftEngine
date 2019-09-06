shader "DepthNormal" {
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        albedoMap("Albedo Map") : texture "2D" = "_whiteTexture"
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
        normalMap("Normal Map") : texture "2D" = "_flatNormalTexture"
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" (shaderDefine)
        heightMap("Height Map") : texture "2D" = "_whiteTexture"
        heightScale("Height Scale") : float range 0.01 0.1 0.001 = "1.0"
    }

    generatePerforatedVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion
    
    glsl_vp {
        $include "DepthNormal.vp"
    }
    glsl_fp {
        $include "DepthNormal.fp"
    }
}