shader "Forward" {
    properties {
        _ALBEDO_SOURCE("Albedo") : enum "Color;Texture" = "0" (shaderDefine)
        diffuseColor("Albedo Color") : vec4 = "1 1 1 1"
        diffuseMap("Albedo Map") : object TextureAsset = "_whiteTexture"
        _NORMAL_SOURCE("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" (shaderDefine)
        normalMap("Normal Map") : object TextureAsset = "_flatNormalTexture"
        detailNormalMap("Detail Normal Map") : object TextureAsset = "_flatNormalTexture"
        detailRepeat("Detail Repeat") : float = "8"
        _WRAPPED_DIFFUSE("Wrapped Diffuse") : bool = "false" (shaderDefine)
        wrapped("Wrapped") : float range 0 1 0.01 = "0.5"
        _SPECULAR_SOURCE("Specular") : enum "None;Color;Texture(RGB);Texture(RGB) + Gloss(A);Color + Gloss(R)" = "0" (shaderDefine)
        specularColor("Specular Color") : vec4 = "1 1 1 1"
        specularMap("Specular Map") : object TextureAsset = "_whiteTexture"
        glossiness("Glossiness") : float range 0 1 0.01 = "0.3"
        //metalness("Metalness") : float range 0 1 0.02 = "0.0"
        //roughness("Roughness") : float range 0 1 0.02 = "0.5"
    }
    
    generatePerforatedVersion
    generateGpuSkinningVersion

    ambientLitVersion "ForwardAmbientLit.shader"
    directLitVersion "ForwardDirectLit.shader"
    ambientLitDirectLitVersion "ForwardAmbientLitDirectLit.shader"
    
    glsl_vp {
        $include "ForwardCore.vp"
    }
    glsl_fp {
        $include "ForwardCore.fp"
    }
}
