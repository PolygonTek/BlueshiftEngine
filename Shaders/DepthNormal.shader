shader "DepthNormal" {
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" [shaderDefine]
        albedoMap("Albedo Map") : texture "2D" = "_whiteTexture" (condition _ALBEDO 1)
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" [shaderDefine]
        normalMap("Normal Map") : texture "2D" = "_flatNormalTexture" (condition _NORMAL 1 2) [normal]
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" [shaderDefine]
        heightMap("Height Map") : texture "2D" = "_whiteTexture" (condition _PARALLAX 1) [linear]
        heightScale("Height Scale") : float range 0.01 0.1 0.001 = "1.0" (condition _PARALLAX 1)
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
