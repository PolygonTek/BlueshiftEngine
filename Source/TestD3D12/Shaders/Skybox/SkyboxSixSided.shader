shader "Skybox/SixSided" {
    skySurface

    stages {
        vertex SM_6_6 "SkyboxSixSided.hlsl" "VSMain"
        fragment SM_6_6 "SkyboxSixSided.hlsl" "PSMain"
    }

    properties {
        skyBackMap("Back (+X)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
        skyFrontMap("Front (-X)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
        skyRightMap("Right (+Y)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
        skyLeftMap("Left (-Y)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
        skyTopMap("Top (+Z)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
        skyDownMap("Down (-Z)") : texture "2D" = "_missingTexture" [noStreaming noMipmaps]
    }
}
