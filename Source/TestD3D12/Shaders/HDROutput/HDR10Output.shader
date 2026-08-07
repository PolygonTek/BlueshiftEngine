shader "Fullscreen/HDR10Output" {
    stages {
        vertex SM_6_6 "HDR10Output.hlsl" "VSMain"
        fragment SM_6_6 "HDR10Output.hlsl" "PSMain"
    }
}
