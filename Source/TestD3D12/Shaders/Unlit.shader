shader "Unlit" {
    stages {
        vertex SM_6_0 "Unlit.hlsl" "VSMain"
        fragment SM_6_0 "Unlit.hlsl" "PSMain"
    }

    properties {
        baseColorTexture("Base Color") : texture "2D" = "_whiteTexture"
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
