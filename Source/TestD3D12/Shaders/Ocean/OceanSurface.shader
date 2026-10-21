shader "Ocean/OceanSurface" {
    stages {
        vertex SM_6_6 "OceanSurface.hlsl" "VSMain"
        fragment SM_6_6 "OceanSurface.hlsl" "PSMain"
    }

    features {
        applySSR {
            APPLY_SSR=1
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

        surfacePrepass {
            SURFACE_PREPASS=1
        }

        transparentSSRPrepass {
            TRANSPARENT_SSR_PREPASS=1
        }
    }
}
