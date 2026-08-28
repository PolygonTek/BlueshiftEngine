#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0), UAV(u1)), " \
    "CBV(b0)"

cbuffer OceanResolveFieldsCB : register(b0) {
    OceanResolveFieldsParams resolveParams;
}

// transposed 2D complex IFFT 결과가 저장된 네 개의 float2 plane 이다.
StructuredBuffer<float2> spatialFields : register(t0);

// cascade c 의 displacement 는 2*c, derivative 는 2*c+1 slice 에 저장한다.
RWTexture2DArray<float4> fieldMap : register(u0);
RWTexture2DArray<float> foamHistoryMap : register(u1);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(OceanResolveFieldsBlockSizeX, OceanResolveFieldsBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint fieldWidth;
    uint fieldHeight;
    uint fieldSliceCount;
    fieldMap.GetDimensions(fieldWidth, fieldHeight, fieldSliceCount);

    uint2 transposedCoord = dispatchThreadId.xy;
    if (transposedCoord.x >= fieldWidth || transposedCoord.y >= fieldWidth) {
        return;
    }

    // spatialFields 를 연속 주소로 읽고 transposed 좌표를 원래 spatial 좌표로 되돌려 출력한다.
    uint2 coord = transposedCoord.yx;
    uint pixelCount = fieldWidth * fieldWidth;
    uint pixelIndex = transposedCoord.y * fieldWidth + transposedCoord.x;

    // centered spectrum 에 대한 IFFT 결과에서 (-1)^(x+y) 를 곱해서 위상 변조를 제거한다.
    float centeredSpectrumSign = ((coord.x + coord.y) & 1u) != 0u ? -1.0f : 1.0f;

    // normalized 2D IFFT 의 1 / (width * height) 정규화를 상쇄하여 물리적인 Fourier mode 합의 크기를 복원한다.
    const float fieldScale = centeredSpectrumSign * float(fieldWidth) * float(fieldWidth);

    float2 Dx_Dy = spatialFields[pixelIndex] * fieldScale;
    float2 Dz_Dxdy = spatialFields[pixelCount + pixelIndex] * fieldScale;
    float2 Dzdx_Dzdy = spatialFields[2u * pixelCount + pixelIndex] * fieldScale;
    float2 Dxdx_Dydy = spatialFields[3u * pixelCount + pixelIndex] * fieldScale;

    // Cascade 합성 단계에서 모든 field 를 더한 뒤 최종 normal 과 Jacobian 을 계산할 수 있도록 원본 값을 보존한다.
    // displacement: (Dx, Dy, Dz, Dxdy), derivative: (Dzdx, Dzdy, Dxdx, Dydy)
    const uint displacementSlice = 2u * resolveParams.cascadeIndex;
    fieldMap[uint3(coord, displacementSlice)] = float4(Dx_Dy, Dz_Dxdy);
    fieldMap[uint3(coord, displacementSlice + 1u)] = float4(Dzdx_Dzdy, Dxdx_Dydy);

    if ((resolveParams.foamOptions & OceanFoamOptionBitEnabled) != 0u) {
        const float displacementX_dy = Dz_Dxdy.y * resolveParams.displacementScale;
        const float displacementX_dx = Dxdx_Dydy.x * resolveParams.displacementScale;
        const float displacementY_dy = Dxdx_Dydy.y * resolveParams.displacementScale;

        const float J = (1.0f + displacementX_dx) * (1.0f + displacementY_dy) - displacementX_dy * displacementX_dy;

        // 수평 변위의 Jacobian 이 작아질수록 파면이 압축되므로 foam source 를 생성한다.
        const float foamSourceRate = max(resolveParams.foamThreshold - J, 0.0f) * resolveParams.foamGenerationRate;
        const uint3 foamCoord = uint3(coord, resolveParams.cascadeIndex);
        const float prevFoam = (resolveParams.foamOptions & OceanFoamOptionBitResetHistory) != 0u ? 0.0f : foamHistoryMap[foamCoord];
        const float foamDecay = exp(-resolveParams.foamDecayRate * resolveParams.deltaTime);
        const float foamIntegrationScale = resolveParams.foamDecayRate > 1e-6f
            ? (1.0f - foamDecay) / resolveParams.foamDecayRate
            : resolveParams.deltaTime;

        foamHistoryMap[foamCoord] = saturate(prevFoam * foamDecay + foamSourceRate * foamIntegrationScale);
    }
}
