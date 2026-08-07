shader "Fullscreen/DisplayP3EDROutput" {
    stages {
        vertex SM_6_6 "DisplayP3EDROutput.hlsl" "VSMain"
        fragment SM_6_6 "DisplayP3EDROutput.hlsl" "PSMain"
    }
}
