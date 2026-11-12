shader "Fog/Composite" {
    program main {
        stages {
            vertex SM_6_6 "FogComposite.hlsl" "VSMain"
            fragment SM_6_6 "FogComposite.hlsl" "PSMain"
        }
    }
}
