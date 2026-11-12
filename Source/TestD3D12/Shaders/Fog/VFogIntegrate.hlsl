#include "VFogCommon.hlsli"

#define VFOG_INTEGRATE_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b1)"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture3D<float4> inputSourceTexture : register(t0);
RWTexture3D<float4> outputIntegratedTexture : register(u0);

[RootSignature(VFOG_INTEGRATE_ROOT_SIGNATURE)]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    const VFogParams vfogParams = cameraConstants.vfogParams;
    if (any(id.xy >= vfogParams.tileCount)) {
        return; 
    }

    // 타일 중심의 화면 UV 를 구하고, 해당 광선의 뷰 공간 단위 깊이당 길이를 계산한다.
    const float2 uv = VFogTileCenterUV(id.xy, vfogParams);
    const float rayScale = VFogRayLengthPerViewDepth(uv, cameraConstants);

    float previousDepth = vfogParams.nearDepth;

    // 누적 초기값, RGB: Lacc (누적 산란/발광 radiance), A: Tacc (누적 투과율)
    float4 accumulated = float4(0, 0, 0, 1);

    // fog 시작 경계 (nearDepth) 에 대한 누적 초기값 (Lacc = 0, Tacc = 1) 을 저장한다.
    outputIntegratedTexture[uint3(id.xy, 0)] = accumulated;

    // 카메라에 가까운 froxel 부터 산란/발광 radiance 와 투과율을 누적한다.
    // 표면 깊이별 fog 합성에 사용할 수 있도록 각 슬라이스 경계까지의 누적값을 저장한다.
    [loop]
    for (uint slice = 0; slice < vfogParams.sliceCount; ++slice) {
        const float nextDepth = VFogSliceToDepth(slice + 1u, vfogParams);
        const float stepLength = max(nextDepth - previousDepth, 0.0f) * rayScale;
        const float4 sourceExtinction = inputSourceTexture.Load(int4(id.xy, slice, 0));
        accumulated = VFogIntegrateSegment(accumulated, sourceExtinction, stepLength);
        outputIntegratedTexture[uint3(id.xy, slice + 1u)] = accumulated;
        previousDepth = nextDepth;
    }
}
