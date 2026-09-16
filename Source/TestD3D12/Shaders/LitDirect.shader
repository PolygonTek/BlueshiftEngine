shader "LitDirect" {
    inheritProperties "Lit"

    stages {
        vertex SM_6_6 "Lit.hlsl" "VSMain"
        fragment SM_6_6 "Lit.hlsl" "PSMain"
    }

    baseDefines {
        DIRECT_LIGHTING=1
    }

    features {
        perforated {
            PERFORATED=1
        }

        outputNormal {
            OUTPUT_NORMAL=1
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
