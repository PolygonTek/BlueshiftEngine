shader "Fullscreen/Image" {
    stages {
        vertex SM_6_6 "FullscreenTriangle.hlsl" "VSMain"
        fragment SM_6_6 "FullscreenTriangle.hlsl" "PSMain"
    }
}
