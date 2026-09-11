#include "DepthCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(DEPTH_ROOT_SIGNATURE)]
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, cameraConstants.surfaceTableIndex, cameraConstants.viewProjMatrix);
}

[RootSignature(DEPTH_ROOT_SIGNATURE)]
void PSMain(DepthPSInput input) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif
}
