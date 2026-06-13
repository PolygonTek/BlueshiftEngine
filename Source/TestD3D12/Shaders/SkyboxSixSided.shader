shader "Skybox/SixSided" {
    skySurface

    stages {
        vertex SM_6_0 "SkyboxSixSided.hlsl" "VSMain"
        fragment SM_6_0 "SkyboxSixSided.hlsl" "PSMain"
    }

    properties {
        tint("Tint") : color3 = "1 1 1"
        exposure("Exposure") : float range 0 8 0.1 = "1.0"
        rotation("Rotation") : float range 0 360 1 = "0"
        skyBackMap("Back (+X)") : texture "2D" = "_defaultTexture" [noStreaming]
        skyFrontMap("Front (-X)") : texture "2D" = "_defaultTexture" [noStreaming]
        skyRightMap("Right (+Y)") : texture "2D" = "_defaultTexture" [noStreaming]
        skyLeftMap("Left (-Y)") : texture "2D" = "_defaultTexture" [noStreaming]
        skyTopMap("Top (+Z)") : texture "2D" = "_defaultTexture" [noStreaming]
        skyDownMap("Down (-Z)") : texture "2D" = "_defaultTexture" [noStreaming]
    }
}
