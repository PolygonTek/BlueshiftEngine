shader "ForwardDirectLit" {
    lighting
    properties {
        _ALBEDO_SOURCE("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        diffuseColor("Albedo Color") : color3 = "1 1 1"
        diffuseMap("Albedo Map") : object TextureAsset = "_whiteTexture"
        _WRAPPED_DIFFUSE("Wrapped Diffuse") : bool = "false" (shaderDefine)
        wrappedDiffuse("Wrapped") : float range 0 1 0.001 = "0.5"
        _SPECULAR_SOURCE("Specular") : enum "None;Color;Texture" = "0" (shaderDefine)
        specularColor("Specular Color") : color3 = "1 1 1"
        specularMap("Specular Map") : object TextureAsset = "_whiteTexture"
        _GLOSS_SOURCE("Gloss") : enum "Glossiness;From Albedo Alpha;From Specular Alpha;Texture(R)" = "0" (shaderDefine)
        glossiness("Glossiness") : float range 0 1 0.001 = "0.2"
        glossMap("Gloss Map") : object TextureAsset = "_whiteTexture"
        //metalness("Metalness") : float range 0 1 0.02 = "0.0"
        //roughness("Roughness") : float range 0 1 0.02 = "0.5"        
        _NORMAL_SOURCE("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
        normalMap("Normal Map") : object TextureAsset = "_flatNormalTexture"
        detailNormalMap("Detail Normal Map") : object TextureAsset = "_flatNormalTexture"
        detailRepeat("Detail Repeat") : float = "8"
        _PARALLAX_SOURCE("Parallax") : enum "None;Texture" = "0" (shaderDefine)
        heightMap("Height Map") : object TextureAsset = "_whiteTexture"
        heightScale("Height Scale") : float range 0.01 1.0 0.001 = "0.008"
        _OCCLUSION_SOURCE("Occlusion") : enum "None;Texture" = "0" (shaderDefine)
        occlusionMap("Occlusion Map") : object TextureAsset = "_whiteTexture"
        occlusionScale("Occlusion Scale") : float range 0 1 0.001 = "1"
        _EMISSION_SOURCE("Emission") : enum "None;Color;Texture" = "0" (shaderDefine)
        emissionMap("Emission Map") : object TextureAsset = "_blackTexture"
        emissionColor("Emission Color") : color3 = "1 1 1"
        emissionScale("Emission Scale") : float range 0 10 0.001 = "1"
    }
    
    generatePerforatedVersion
    generateGpuSkinningVersion
    generateParallelShadowVersion
    generateSpotShadowVersion
    generatePointShadowVersion

    glsl_vp {
        #define DIRECT_LIGHTING 1
        $include "ForwardCore.vp"
    }
    glsl_fp {
        #define DIRECT_LIGHTING 1
        $include "ForwardCore.fp"
    }
}
