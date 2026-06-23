#ifndef _TRANSPOSECOMPLEX_HLSLI_
#define _TRANSPOSECOMPLEX_HLSLI_

#include "../ShaderInterop.h"

#if defined(TRANSPOSE_COMPLEX_SIZE)
#define TRANSPOSE_COMPLEX_INPUT_WIDTH  TRANSPOSE_COMPLEX_SIZE
#define TRANSPOSE_COMPLEX_INPUT_HEIGHT TRANSPOSE_COMPLEX_SIZE
#endif

#ifndef TRANSPOSE_COMPLEX_INPUT_WIDTH
#error TRANSPOSE_COMPLEX_INPUT_WIDTH must be defined.
#endif

#ifndef TRANSPOSE_COMPLEX_INPUT_HEIGHT
#error TRANSPOSE_COMPLEX_INPUT_HEIGHT must be defined.
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

StructuredBuffer<float2> inputBuffer : register(t0);
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 tile[TransposeComplexTileSize][TransposeComplexTileSize + 1];

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(TransposeComplexTileSize, TransposeComplexTileSize, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    // input:  TRANSPOSE_COMPLEX_INPUT_WIDTH x TRANSPOSE_COMPLEX_INPUT_HEIGHT
    // output: TRANSPOSE_COMPLEX_INPUT_HEIGHT x TRANSPOSE_COMPLEX_INPUT_WIDTH
    uint2 srcCoord = groupId.xy * TransposeComplexTileSize + groupThreadId.xy;
    uint srcIndex = srcCoord.y * TRANSPOSE_COMPLEX_INPUT_WIDTH + srcCoord.x;

    tile[groupThreadId.y][groupThreadId.x] = inputBuffer[srcIndex];

    GroupMemoryBarrierWithGroupSync();

    uint2 dstCoord = groupId.yx * TransposeComplexTileSize + groupThreadId.xy;
    uint dstIndex = dstCoord.y * TRANSPOSE_COMPLEX_INPUT_HEIGHT + dstCoord.x;

    outputBuffer[dstIndex] = tile[groupThreadId.x][groupThreadId.y];
}

#endif
