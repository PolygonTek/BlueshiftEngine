shader "Skybox/SixSided" {
    skySurface

    stages {
        vertex SM_6_6 "SkyboxSixSided.hlsl" "VSMain"
        fragment SM_6_6 "SkyboxSixSided.hlsl" "PSMain"
    }

    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyBackMap("Back (+X)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
        skyFrontMap("Front (-X)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
        skyRightMap("Right (+Y)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
        skyLeftMap("Left (-Y)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
        skyTopMap("Top (+Z)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
        skyDownMap("Down (-Z)") : texture "2D" = "_defaultTexture" [noStreaming noMipmaps]
    }
}
