shader "Skybox/Cubemap" {
    skySurface

    properties {
        skyCubeMap("Cube Map") : texture "CUBE" = "Data/EngineTextures/default_sky.dds" [noStreaming noMipmaps]
    }

    program main {
        stages {
            vertex SM_6_6 "SkyboxCubemap.hlsl" "VSMain"
            fragment SM_6_6 "SkyboxCubemap.hlsl" "PSMain"
        }
    }
}
