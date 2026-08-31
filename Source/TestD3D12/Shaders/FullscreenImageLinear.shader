shader "Fullscreen/ImageLinear" {
    stages {
        vertex SM_6_6 "FullscreenTriangle.hlsl" "VSMain"
        fragment SM_6_6 "FullscreenTriangle.hlsl" "PSLinear"
    }
}
