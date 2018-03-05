shader "Lit/StandardSpec" {
    litSurface
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        albedoColor("Albedo Color") : color3 = "1 1 1"
        albedoAlpha("Albedo Alpha") : float range 0 1.0 0.001 = "1"
        albedoMap("Albedo Map") : texture = "_whiteTexture"
        _SPECULAR("Specular") : enum "Color;Texture" = "0" (shaderDefine)
        specularColor("Specular Color") : color3 = "0 0 0"
        specularMap("Specular Map") : texture = "_whiteTexture"
        _GLOSS("Gloss") : enum "Scale;From Albedo Alpha;From Specular Alpha;Texture (R)" = "0" (shaderDefine)
        glossMap("Gloss Map") : texture = "_whiteTexture"
        glossScale("Gloss Scale") : float range 0 1.0 0.001 = "0.0"
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
        normalMap("Normal Map") : texture = "_flatNormalTexture"
        detailNormalMap("Detail Normal Map") : texture = "_flatNormalTexture"
        detailRepeat("Detail Repeat") : float = "8"
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" (shaderDefine)
        heightMap("Height Map") : texture = "_whiteTexture"
        heightScale("Height Scale") : float range 0.01 1.0 0.001 = "0.008"
        _OCCLUSION("Occlusion") : enum "None;Texture (R);From Metallic Map (B)" = "0" (shaderDefine)
        occlusionMap("Occlusion Map") : texture = "_whiteTexture"
        occlusionStrength("Occlusion Strength") : float range 0 1 0.001 = "1"
        _EMISSION("Emission") : enum "None;Color;Texture" = "0" (shaderDefine)
        emissionColor("Emission Color") : color3 = "1 1 1"
        emissionMap("Emission Map") : texture = "_blackTexture"
        emissionScale("Emission Scale") : float range 0 16 0.001 = "1"
    }
    
    generatePerforatedVersion
    generatePremulAlphaVersion
    generateGpuSkinningVersion

    ambientLitVersion "StandardSpecAmbientLit.shader"
    directLitVersion "StandardSpecDirectLit.shader"
    ambientLitDirectLitVersion "StandardSpecAmbientLitDirectLit.shader"
    
    glsl_vp {
        $include "StandardCore.vp"
    }
    glsl_fp {
        $include "StandardCore.fp"
    }
}
