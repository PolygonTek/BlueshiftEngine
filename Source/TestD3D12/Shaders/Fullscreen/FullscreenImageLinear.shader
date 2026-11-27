shader "Fullscreen/ImageLinear" {
    program main {
        stages {
            vertex SM_6_6 "FullscreenImage.hlsl" "VSMain"
            fragment SM_6_6 "FullscreenImage.hlsl" "PSLinear"
        }
    }
}
