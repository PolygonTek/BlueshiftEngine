shader "Lit" {
    stages {
        vertex SM_6_6 "Lit.hlsl" "VSMain"
        fragment SM_6_6 "Lit.hlsl" "PSMain"
    }

    properties {
        _BASECOLOR("Base Color") : enum "Color;Texture" = "1" [shaderDefine]
        baseColorFactor("Base Color Factor") : color4 = "1 1 1 1"
        baseColorMap("Base Color Map") : texture "2D" = "_whiteTexture" (condition _BASECOLOR 1)
        _METALLIC("Metallic") : enum "Factor;Texture (R);Texture (G);Texture (B);Texture (A)" = "0" [shaderDefine]
        metallicMap("Metallic Map") : texture "2D" = "_whiteTexture" (condition _METALLIC 1 2 3 4) [linear]
        metallicFactor("Metallic Factor") : float range 0 1 0.01 = "1.0"
        _ROUGHNESS("Roughness") : enum "Factor;Texture (R);Inverted Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" [shaderDefine]
        roughnessMap("Roughness Map") : texture "2D" = "_whiteTexture" (condition _ROUGHNESS 1 2) [linear]
        roughnessFactor("Roughness Factor") : float range 0 1 0.01 = "1.0"
        _NORMAL("Normal") : enum "Vertex;Texture;Texture + Detail Texture" = "0" [shaderDefine]
        normalMap("Normal Map") : texture "2D" = "_flatNormalTexture" (condition _NORMAL 1 2) [normal]
        _CLEARCOAT("Clear Coat") : enum "None;Factor;Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" [shaderDefine]
        clearCoatMap("Clear Coat Map") : texture "2D" = "_whiteTexture" (condition _CLEARCOAT 2) [linear]
        clearCoatFactor("Clear Coat Factor") : float range 0 1 0.01 = "1.0" (condition _CLEARCOAT 1 2 3 4 5 6)
        _CLEARCOAT_ROUGHNESS("Clear Coat Roughness") : enum "Factor;Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" (condition _CLEARCOAT 1 2 3 4 5 6) [shaderDefine]
        clearCoatRoughnessMap("Clear Coat Roughness Map") : texture "2D" = "_whiteTexture" (condition _CLEARCOAT_ROUGHNESS 1) [linear]
        clearCoatRoughnessFactor("Clear Coat Roughness Factor") : float range 0 1 0.01 = "1.0" (condition _CLEARCOAT 1 2 3 4 5 6)
        _CLEARCOAT_NORMAL("Clear Coat Normal") : enum "Surface Normal;Texture" = "0" (condition _CLEARCOAT 1 2 3 4 5 6) [shaderDefine]
        clearCoatNormalMap("Clear Coat Normal Map") : texture "2D" = "_flatNormalTexture" (condition _CLEARCOAT_NORMAL 1) [normal]
        _PARALLAX("Parallax") : enum "None;Texture (R)" = "0" [shaderDefine]
        heightMap("Height Map") : texture "2D" = "_whiteTexture" (condition _PARALLAX 1) [linear]
        heightScale("Height Scale") : float range 0 1 0.001 = "0.0" (condition _PARALLAX 1)
        _AO("AO") : enum "None;Texture (R);From Metallic Map (R);From Metallic Map (G);From Metallic Map (B);From Metallic Map (A)" = "0" [shaderDefine]
        aoMap("AO Map") : texture "2D" = "_whiteTexture" (condition _AO 1) [linear]
        aoStrength("AO Strength") : float range 0 1 0.001 = "1.0" (condition _AO 1 2 3 4 5)
        _EMISSION("Emission") : enum "None;Color;Texture" = "0" [shaderDefine]
        emissionColor("Emission Color") : color3 = "1 1 1" (condition _EMISSION 1 2)
        emissiveMap("Emissive Map") : texture "2D" = "_blackTexture" (condition _EMISSION 2)
        emissionFactor("Emission Factor") : float range 0 16 0.001 = "1.0" (condition _EMISSION 1 2)
    }

    features {
        outputNormal {
            OUTPUT_NORMAL=1
        }

        outputVelocity {
            OUTPUT_VELOCITY=1
        }

        gpuSkinningWeight1 {
            GPU_SKINNING=1
            GPU_SKINNING_WEIGHTS=1
        }

        gpuSkinningWeight4 {
            GPU_SKINNING=1
            GPU_SKINNING_WEIGHTS=4
        }

        gpuSkinningWeight8 {
            GPU_SKINNING=1
            GPU_SKINNING_WEIGHTS=8
        }
    }
}
