shader "Debug/Draw" {
    stages {
        vertex SM_6_0 "DebugDraw.hlsl" "VSMain"
        fragment SM_6_0 "DebugDraw.hlsl" "PSMain"
    }
}
