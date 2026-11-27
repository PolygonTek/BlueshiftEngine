shader "Unlit" {
    properties {
        tcScale("Texture Coordinate Scale") : vec2 = "1 1"
        tcTranslation("Texture Coordinate Translation") : vec2 = "0 0"
        _EMISSIVE("Emissive") : enum "Color;Texture" = "1" [shaderDefine]
        emissiveColor("Emissive Color") : color3 = "1 1 1"
        emissiveMap("Emissive Map") : texture "2D" = "_whiteTexture" (condition _EMISSIVE 1)
        emissiveFactor("Emissive Factor") : float = "1.0"
        opacity("Opacity") : float range 0 1 0.001 = "1.0"
        alphaCutoff("Alpha Cutoff") : float range 0 1 0.001 = "0.5"
    }

    program prepass {
        propertyDependencies {
            _EMISSIVE
        }

        stages {
            vertex SM_6_6 "UnlitPrepass.hlsl" "VSMain"
            fragment SM_6_6 "UnlitPrepass.hlsl" "PSMain" (features perforated dithered ditherFade outputNormal outputRoughness outputVelocity)
        }

        features {
            perforated {
                PERFORATED=1
            }

            dithered {
                DITHERED=1
            }

            ditherFade {
                DITHER_FADE=1
            }

            outputNormal {
                OUTPUT_NORMAL=1
            }

            outputRoughness {
                OUTPUT_ROUGHNESS=1
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

    program shadow {
        baseDefines { SHADOW_ALPHA_FROM_EMISSIVE = 1 }

        propertyDependencies {
            _EMISSIVE
        }

        stages {
            vertex SM_6_6 "../Shadow/ShadowDepth.hlsl" "VSMain"
            fragment SM_6_6 "../Shadow/ShadowDepth.hlsl" "PSMain" (features perforated dithered ditherFade)
        }

        features {
            perforated {
                PERFORATED=1
            }

            dithered {
                DITHERED=1
            }

            ditherFade {
                DITHER_FADE=1
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

    program main {
        stages {
            vertex SM_6_6 "Unlit.hlsl" "VSMain"
            fragment SM_6_6 "Unlit.hlsl" "PSMain"
        }

        features {
            perforated {
                PERFORATED=1
            }

            dithered {
                DITHERED=1
            }

            ditherFade {
                DITHER_FADE=1
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
}
