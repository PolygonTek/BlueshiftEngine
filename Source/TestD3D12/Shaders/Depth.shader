shader "Depth" {
    stages {
        vertex SM_6_6 "Depth.hlsl" "VSMain"
        fragment SM_6_6 "Depth.hlsl" "PSMain"
    }

    features {
        perforated {
            PERFORATED=1
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
