#ifndef _TRANSPOSE_COMPLEX_HLSLI_
#define _TRANSPOSE_COMPLEX_HLSLI_

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

// TRANSPOSE_COMPLEX_INPUT_WIDTH x TRANSPOSE_COMPLEX_INPUT_HEIGHT 행렬이 row-major 로 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// TRANSPOSE_COMPLEX_INPUT_HEIGHT x TRANSPOSE_COMPLEX_INPUT_WIDTH 행렬이 row-major 로 연속으로 배치될 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// +1 padding 은 transpose 된 타일을 읽을 때 shared memory bank conflict 를 줄이기 위한 것이다.
groupshared float2 tile[TransposeComplexTileSize][TransposeComplexTileSize + 1];

// 스레드 그룹 하나가 16x16 타일 하나를 처리한다.
// 타일을 shared memory 에 올린 뒤, 타일 내부를 transpose 해서 전치된 타일 위치에 쓴다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(TransposeComplexTileSize, TransposeComplexTileSize, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    uint matrixElementCount = TRANSPOSE_COMPLEX_INPUT_WIDTH * TRANSPOSE_COMPLEX_INPUT_HEIGHT;
    uint matrixBase = groupId.z * matrixElementCount;

    uint2 srcCoord = groupId.xy * TransposeComplexTileSize + groupThreadId.xy;
    uint srcIndex = matrixBase + srcCoord.y * TRANSPOSE_COMPLEX_INPUT_WIDTH + srcCoord.x;

    tile[groupThreadId.y][groupThreadId.x] = inputBuffer[srcIndex];

    GroupMemoryBarrierWithGroupSync();

    uint2 dstCoord = groupId.yx * TransposeComplexTileSize + groupThreadId.xy;
    uint dstIndex = matrixBase + dstCoord.y * TRANSPOSE_COMPLEX_INPUT_HEIGHT + dstCoord.x;

    outputBuffer[dstIndex] = tile[groupThreadId.x][groupThreadId.y];
}

#endif // _TRANSPOSE_COMPLEX_HLSLI_
