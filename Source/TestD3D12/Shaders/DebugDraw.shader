shader "Debug/Draw" {
    program main {
        stages {
            vertex SM_6_6 "DebugDraw.hlsl" "VSMain"
            fragment SM_6_6 "DebugDraw.hlsl" "PSMain"
        }
    }
}
