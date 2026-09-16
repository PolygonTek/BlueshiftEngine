#include "DepthCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(DEPTH_ROOT_SIGNATURE)]
#ifdef OUTPUT_VELOCITY
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, cameraConstants.surfaceTableIndex, cameraConstants.viewProjMatrix, cameraConstants.unjitteredViewProjMatrix, cameraConstants.prevUnjitteredViewProjMatrix);
}
#else
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, cameraConstants.surfaceTableIndex, cameraConstants.viewProjMatrix);
}
#endif

[RootSignature(DEPTH_ROOT_SIGNATURE)]
#ifdef OUTPUT_VELOCITY
float2 PSMain(DepthPSInput input) : SV_Target0 {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif
    return CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
}
#else
void PSMain(DepthPSInput input) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif
}
#endif
