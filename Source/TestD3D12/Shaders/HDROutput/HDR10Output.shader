shader "Fullscreen/HDR10Output" {
    program main {
        stages {
            vertex SM_6_6 "HDR10Output.hlsl" "VSMain"
            fragment SM_6_6 "HDR10Output.hlsl" "PSMain"
        }
    }
}
