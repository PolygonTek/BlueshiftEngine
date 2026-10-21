shader "Unlit" {
    stages {
        vertex SM_6_6 "Unlit.hlsl" "VSMain"
        fragment SM_6_6 "Unlit.hlsl" "PSMain"
    }

    properties {
        tcScale("Texture Coordinate Scale") : vec2 = "1 1"
        tcTranslation("Texture Coordinate Translation") : vec2 = "0 0"
        baseColorMap("Base Color") : texture "2D" = "_whiteTexture"
    }

    features {
        outputNormal {
            OUTPUT_NORMAL=1
        }

        outputRoughness {
            OUTPUT_ROUGHNESS=1
        }

        outputVelocity {
            OUTPUT_VELOCITY=1
        }

        surfacePrepass {
            SURFACE_PREPASS=1
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
