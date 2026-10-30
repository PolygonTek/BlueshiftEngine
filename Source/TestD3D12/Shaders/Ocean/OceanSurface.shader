shader "Ocean/OceanSurface" {
    program prepass {
        stages {
            vertex SM_6_6 "OceanPrepass.hlsl" "VSMain"
            fragment SM_6_6 "OceanPrepass.hlsl" "PSMain" (features outputNormal outputRoughness outputVelocity)
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

            transparentSSRPrepass {
                TRANSPARENT_SSR_PREPASS=1
            }
        }
    }

    program main {
        stages {
            vertex SM_6_6 "OceanSurface.hlsl" "VSMain"
            fragment SM_6_6 "OceanSurface.hlsl" "PSMain"
        }

        features {
            indirectLighting {
                INDIRECT_LIGHTING=1
            }

            outputVelocity {
                OUTPUT_VELOCITY=1
            }
        }
    }
}
