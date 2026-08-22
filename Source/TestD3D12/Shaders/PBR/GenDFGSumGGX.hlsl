#include "../ShaderInterop.h"
#include "IBLIntegration.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(UAV(u0))"

RWTexture2D<float2> outputDFG : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(IBLIntegrationBlockSizeX, IBLIntegrationBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint outputWidth;
    uint outputHeight;
    outputDFG.GetDimensions(outputWidth, outputHeight);

    const uint2 outputCoord = dispatchThreadId.xy;
    if (outputCoord.x >= outputWidth || outputCoord.y >= outputHeight) {
        return;
    }

    // Match the texel-center coordinates produced by the legacy fullscreen pass.
    const float2 uv = (float2(outputCoord) + 0.5f) / float2(outputWidth, outputHeight);
    outputDFG[outputCoord] = IntegrateDFG(uv.x, uv.y);
}
