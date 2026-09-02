shader "SceneSnapshotDownsample" {
    stages {
        vertex SM_6_6 "SceneSnapshotDownsample.hlsl" "VSMain"
        fragment SM_6_6 "SceneSnapshotDownsample.hlsl" "PSMain"
    }
}
