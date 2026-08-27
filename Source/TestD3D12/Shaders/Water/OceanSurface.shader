shader "Water/OceanSurface" {
    stages {
        vertex SM_6_6 "OceanSurface.hlsl" "VSMain"
        fragment SM_6_6 "OceanSurface.hlsl" "PSMain"
    }

    features {
        outputNormal {
            OUTPUT_NORMAL=1
        }

        outputVelocity {
            OUTPUT_VELOCITY=1
        }
    }
}
