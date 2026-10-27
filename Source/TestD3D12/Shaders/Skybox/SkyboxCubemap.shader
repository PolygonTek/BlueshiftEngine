shader "Skybox/Cubemap" {
    skySurface

    stages {
        vertex SM_6_6 "SkyboxCubemap.hlsl" "VSMain"
        fragment SM_6_6 "SkyboxCubemap.hlsl" "PSMain"
    }

    properties {
        skyCubeMap("Cube Map") : texture "CUBE" = "Data/EngineTextures/default_sky.dds" [noStreaming noMipmaps]
    }
}
