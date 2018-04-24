shader "Lit/Standard" {
    litSurface
    properties {
        _ALBEDO("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        albedoColor("Albedo Color") : color3 = "1 1 1"
        albedoAlpha("Albedo Alpha") : float range 0 1.0 0.001 = "1"
        albedoMap("Albedo Map") : texture = "_whiteTexture"
        _METALLIC("Metallic") : enum "Scale;Texture (R);Texture (G);Texture (B);Texture (A)" = "0" (shaderDefine)
        metallicMap("Metallic Map") : texture = "_whiteTexture"
        metallicScale("Metallic Scale") : float range 0 1.0 0.01 = "1.0"
        _ROUGHNESS("Roughness") : enum "Scale;Texture (R);Inverted Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" (shaderDefine)
        roughnessMap("Roughness Map") : texture = "_whiteTexture"
        roughnessScale("Roughness Scale") : float range 0 1.0 0.01 = "1.0"
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
        normalMap("Normal Map") : texture = "_flatNormalTexture"
        detailNormalMap("Detail Normal Map") : texture = "_flatNormalTexture"
        detailRepeat("Detail Repeat") : float = "8"
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" (shaderDefine)
        heightMap("Height Map") : texture = "_whiteTexture"
        heightScale("Height Scale") : float range 0.01 1.0 0.001 = "0.008"
        _OCCLUSION("Occlusion") : enum "None;Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" (shaderDefine)
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
    generateGpuInstancingVersion

    ambientLitVersion "StandardAmbientLit"
    directLitVersion "StandardDirectLit"
    ambientLitDirectLitVersion "StandardAmbientLitDirectLit"
    
    glsl_vp {
        $include "StandardCore.vp"
    }
    glsl_fp {
        $include "StandardCore.fp"
    }
}
