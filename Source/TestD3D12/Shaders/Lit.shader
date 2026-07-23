shader "Lit" {
    stages {
        vertex SM_6_0 "Lit.hlsl" "VSMain"
        fragment SM_6_0 "Lit.hlsl" "PSMain"
    }

    properties {
        baseColorTexture("Base Color") : texture "2D" = "_whiteTexture"
        metallicTexture("Metallic") : texture "2D" = "_whiteTexture" [linear]
        roughnessTexture("Roughness") : texture "2D" = "_whiteTexture" [linear]
        metallicFactor("Metallic Factor") : float range 0 1 0.01 = "1.0"
        roughnessFactor("Roughness Factor") : float range 0 1 0.01 = "1.0"
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
