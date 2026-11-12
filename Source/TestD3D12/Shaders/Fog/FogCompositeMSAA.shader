shader "Fog/CompositeMSAA" {
    program main {
        baseDefines { FOG_MSAA = 1 }
        stages {
            vertex SM_6_6 "FogComposite.hlsl" "VSMain"
            fragment SM_6_6 "FogComposite.hlsl" "PSMain"
        }
    }
}
