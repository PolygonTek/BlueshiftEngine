#include "../ShaderInterop.h"
#include "../Common.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer HiZMipGenCB : register(b0) {
    HiZMipGenParams hiZMipGenParams;
}

Texture2D<float> inputTexture : register(t0);
RWTexture2D<float> outputMip : register(u0);

float ReduceDepth(float a, float b) {
    if (hiZMipGenParams.useMinDepth != 0) {
        return min(a, b);
    }
    return max(a, b);
}

float ReduceInputFootprint(uint2 outputCoord) {
    const uint2 inputSize = hiZMipGenParams.inputSize;

    // Hi-Z mips use floor(input / 2): even axes reduce 2 texels, odd axes overlap one texel and reduce 3.
    const uint2 srcMin = outputCoord * 2;
    const uint2 srcSize = min(uint2(2, 2) + (inputSize & uint2(1, 1)), inputSize);

    float depth = inputTexture.Load(int3(srcMin, 0));

    if (srcSize.x > 1) {
        const uint2 srcCoord10 = srcMin + uint2(1, 0);
        depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord10, 0)));

        if (srcSize.x > 2) {
            const uint2 srcCoord20 = srcMin + uint2(2, 0);
            depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord20, 0)));
        }
    }

    if (srcSize.y > 1) {
        const uint2 srcCoord01 = srcMin + uint2(0, 1);
        depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord01, 0)));

        if (srcSize.x > 1) {
            const uint2 srcCoord11 = srcMin + uint2(1, 1);
            depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord11, 0)));
        }

        if (srcSize.x > 2) {
            const uint2 srcCoord21 = srcMin + uint2(2, 1);
            depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord21, 0)));
        }

        if (srcSize.y > 2) {
            const uint2 srcCoord02 = srcMin + uint2(0, 2);
            depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord02, 0)));

            if (srcSize.x > 1) {
                const uint2 srcCoord12 = srcMin + uint2(1, 2);
                depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord12, 0)));
            }

            if (srcSize.x > 2) {
                const uint2 srcCoord22 = srcMin + uint2(2, 2);
                depth = ReduceDepth(depth, inputTexture.Load(int3(srcCoord22, 0)));
            }
        }
    }

    return depth;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(GenHiZMipBlockSizeX, GenHiZMipBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputCoord = dispatchThreadId.xy;
    if (any(outputCoord >= hiZMipGenParams.outputSize)) {
        return;
    }

    outputMip[outputCoord] = ReduceInputFootprint(outputCoord);
}
