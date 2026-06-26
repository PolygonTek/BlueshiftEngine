shader "Fullscreen/Image" {
    stages {
        vertex SM_6_0 "FullscreenTriangle.hlsl" "VSMain"
        fragment SM_6_0 "FullscreenTriangle.hlsl" "PSMain"
    }
}
