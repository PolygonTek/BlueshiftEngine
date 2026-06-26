shader "FullScreen/Image" {
    stages {
        vertex SM_6_0 "FullScreenTriangle.hlsl" "VSMain"
        fragment SM_6_0 "FullScreenTriangle.hlsl" "PSMain"
    }
}
