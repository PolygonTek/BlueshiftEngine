shader "Lit/StandardSpec" {
    litSurface
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" [shaderDefine]
        albedoColor("Albedo Color") : color3 = "1 1 1" (condition _ALBEDO 0)
        albedoAlpha("Albedo Alpha") : float range 0 1.0 0.001 = "1" (condition _ALBEDO 0)
        albedoMap("Albedo Map") : texture "2D" = "_whiteTexture" (condition _ALBEDO 1)
        _SPECULAR("Specular") : enum "Color;Texture" = "0" [shaderDefine]
        specularColor("Specular Color") : color3 = "0.047 0.047 0.047" (condition _SPECULAR 0)
        specularAlpha("Specular Alpha") : float range 0 1.0 0.001 = "1" (condition _SPECULAR 0)
        specularMap("Specular Map") : texture "2D" = "_whiteTexture" (condition _SPECULAR 1)
        _GLOSS("Gloss") : enum "Scale;From Albedo Alpha;From Specular Alpha;Texture (R)" = "0" [shaderDefine]
        glossMap("Gloss Map") : texture "2D" = "_whiteTexture" (condition _GLOSS 3)
        glossScale("Gloss Scale") : float range 0 1.0 0.001 = "0.0"
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" [shaderDefine]
        normalMap("Normal Map") : texture "2D" = "_flatNormalTexture" (condition _NORMAL 1 2)
        detailNormalMap("Detail Normal Map") : texture "2D" = "_flatNormalTexture" (condition _NORMAL 2)
        detailRepeat("Detail Repeat") : float = "8" (condition _NORMAL 2)
        /*_ANISO("Anisotropy") : enum "None;Value;Texture" = "0" [shaderDefine]
        anisotropyMap("Anisotropy Map") : texture "2D" = "_flatNormalTexture" (condition _ANISO 2)
        anisotropy("Anisotropy") : float range - 1.0 1.0 0.01 = "0.0" (condition _ANISO 1 2)*/
        _CLEARCOAT("Clear Coat") : enum "None;Scale;Texture (R);From Albedo Alpha;From Specular Alpha" = "0" [shaderDefine]
        clearCoatMap("Clear Coat Map") : texture "2D" = "_whiteTexture" (condition _CLEARCOAT 2)
        clearCoatScale("Clear Coat Scale") : float range 0 1.0 0.01 = "1.0" (condition _CLEARCOAT 1 2 3 4)
        _CC_ROUGHNESS("Clear Coat Roughness") : enum "Scale;Texture (R);From Albedo Alpha;From Specular Alpha" = "0" (condition _CLEARCOAT 1 2 3 4) [shaderDefine]
        clearCoatRoughnessMap("Clear Coat Roughness Map") : texture "2D" = "_whiteTexture" (condition _CLEARCOAT 1 2 3 4 _CC_ROUGHNESS 1)
        clearCoatRoughnessScale("Clear Coat Roughness Scale") : float range 0 1.0 0.01 = "0.0" (condition _CLEARCOAT 1 2 3 4)
        _CC_NORMAL("Clear Coat Normal") : enum "Surface Normal;Texture" = "0" (condition _CLEARCOAT 1 2 3 4) [shaderDefine]
        clearCoatNormalMap("Clear Coat Normal Map") : texture "2D" = "_flatNormalTexture" (condition _CLEARCOAT 1 2 3 4 _CC_NORMAL 1)
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" [shaderDefine]
        heightMap("Height Map") : texture "2D" = "_whiteTexture" (condition _PARALLAX 1)
        heightScale("Height Scale") : float range 0.01 0.1 0.001 = "1." (condition _PARALLAX 1)
        _OCC("Occlusion") : enum "None;Texture (R);From Albedo Map (A);From Specular Map (A)" = "0" [shaderDefine]
        occlusionMap("Occlusion Map") : texture "2D" = "_whiteTexture" (condition _OCC 1)
        occlusionStrength("Occlusion Strength") : float range 0 1 0.001 = "1" (condition _OCC 1 2 3)
        _EMISSION("Emission") : enum "None;Color;Texture" = "0" [shaderDefine]
        emissionColor("Emission Color") : color3 = "1 1 1" (condition _EMISSION 1)
        emissionMap("Emission Map") : texture "2D" = "_blackTexture" (condition _EMISSION 2)
        emissionScale("Emission Scale") : float range 0 16 0.001 = "1" (condition _EMISSION 1 2)
    }

    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion
    generateGpuInstancingVersion

    indirectLitVersion "StandardSpecIndirectLit"
    directLitVersion "StandardSpecDirectLit"
    indirectLitDirectLitVersion "StandardSpecIndirectLitDirectLit"
    
    glsl_vp {
        $include "StandardCore.vp"
    }
    glsl_fp {
        $include "StandardCore.fp"
    }
}
