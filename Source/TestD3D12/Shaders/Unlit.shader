shader "Unlit" {
    properties {
        tcScale("Texture Coordinate Scale") : vec2 = "1 1"
        tcTranslation("Texture Coordinate Translation") : vec2 = "0 0"
        _BASECOLOR("Base Color") : enum "Color;Texture" = "1" [shaderDefine]
        baseColorMap("Base Color Map") : texture "2D" = "_whiteTexture" (condition _BASECOLOR 1)
        baseColorFactor("Base Color Factor") : color4 = "1 1 1 1"
        alphaCutoff("Alpha Cutoff") : float range 0 1 0.001 = "0.5"
    }

    program prepass {
        propertyDependencies {
            _BASECOLOR
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
        propertyDependencies {
            _BASECOLOR
        }

        stages {
            vertex SM_6_6 "ShadowDepth.hlsl" "VSMain"
            fragment SM_6_6 "ShadowDepth.hlsl" "PSMain" (features perforated dithered ditherFade)
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
