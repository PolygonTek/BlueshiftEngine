#include "DepthCommon.hlsli"

cbuffer ShadowViewCB : register(b1) {
    ShadowViewConstants shadowViewConstants;
};

[RootSignature(DEPTH_ROOT_SIGNATURE)]
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, shadowViewConstants);
}

[RootSignature(DEPTH_ROOT_SIGNATURE)]
void PSMain(DepthPSInput input) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif
}
