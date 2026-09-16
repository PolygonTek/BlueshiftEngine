#include "../ShaderInterop.h"
#include "../Blur.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "RootConstants(num32BitConstants=4, b0), " \
    "CBV(b1)"

cbuffer AOBilateralCB : register(b0) {
    AOBilateralParams aoBilateralParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

Texture2D<float> inputAOTexture : register(t0);
Texture2D<float> inputDepthTexture : register(t1);
RWTexture2D<float> outputAOTexture : register(u0);

float FilterAO(uint2 pixelCoord, int2 axis) {
    // AO writes 1 for background; avoid reading its known value from the texture.
    if (inputDepthTexture.Load(int3(pixelCoord, 0)) == aoBilateralParams.farPlaneDeviceDepth) {
        return 1.0f;
    }

    BlurBilateralParams params;
    params.inputSize = aoBilateralParams.outputSize;
    params.depthSigmaRcp = aoBilateralParams.depthSigmaRcp;
    params.farPlaneDeviceDepth = aoBilateralParams.farPlaneDeviceDepth;
    params.ndcDepthScaleBias = cameraConstants.ndcDepthScaleBias;
    params.projectionZ = cameraConstants.projectionZ;
    params.orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;

    return saturate(BilateralBlur(inputAOTexture, inputDepthTexture, pixelCoord, axis, params));
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(AOBilateralBlockSizeX, AOBilateralBlockSizeY, 1)]
void CSMainHorizontal(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= aoBilateralParams.outputSize)) {
        return;
    }

    outputAOTexture[pixelCoord] = FilterAO(pixelCoord, int2(1, 0));
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(AOBilateralBlockSizeX, AOBilateralBlockSizeY, 1)]
void CSMainVertical(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoord = dispatchThreadId.xy;
    if (any(pixelCoord >= aoBilateralParams.outputSize)) {
        return;
    }

    outputAOTexture[pixelCoord] = FilterAO(pixelCoord, int2(0, 1));
}
