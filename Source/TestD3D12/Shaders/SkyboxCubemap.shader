shader "Skybox/Cubemap" {
    skySurface

    stages {
        vertex SM_6_6 "Skybox.hlsl" "VSMain"
        fragment SM_6_6 "Skybox.hlsl" "PSMain"
    }

    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyCubeMap("Cube Map") : texture "CUBE" = "Data/EngineTextures/default_sky.dds" [noStreaming noMipmaps]
    }
}
