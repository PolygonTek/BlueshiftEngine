#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "OceanSpectrum.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer OceanTimeSpectrumCB : register(b0) {
    OceanTimeSpectrumParams spectrumParams;
}

// RG: h0(k), BA: conj(h0(-k))
Texture2DArray<float4> initialSpectrum : register(t0);

// 8개의 실수 field spectrum 을 Z = A + iB 로 두 개씩 묶은 4개의 complex plane.
// 각 plane 은 N * N 개의 float2 로 구성되며 일반 row-major 순서로 저장한다.
RWStructuredBuffer<float2> currentSpectrum : register(u0);

// 각 float2 는 복소 스펙트럼의 (real, imaginary) 성분을 저장한다.
struct OceanFieldSpectra {
    float2 displacementX;       // x 축 방향 변위 스펙트럼
    float2 displacementY;       // y 축 방향 변위 스펙트럼
    float2 displacementZ;       // z 축 방향 변위 스펙트럼
    float2 displacementX_dx;    // ∂Dx/∂x 스펙트럼
    float2 displacementY_dy;    // ∂Dy/∂y 스펙트럼
    float2 displacementX_dy;    // ∂Dx/∂y = ∂Dy/∂x 스펙트럼
    float2 displacementZ_dx;    // ∂Dz/∂x 스펙트럼
    float2 displacementZ_dy;    // ∂Dz/∂y 스펙트럼
};

OceanFieldSpectra ComputeOceanFieldSpectra(uint2 pq, uint spectrumSize) {
    OceanFieldSpectra spectra = (OceanFieldSpectra)0;

    float2 waveVector = GetWaveVector(pq, spectrumSize, spectrumParams.patchSize);
    float waveNumberSquared = dot(waveVector, waveVector);
    // wave vector 가 0 인 경우, 모든 field spectrum 은 0 이 된다.
    if (waveNumberSquared <= 0.0f) {
        return spectra;
    }

    float invK = rsqrt(waveNumberSquared);
    float kLength = waveNumberSquared * invK;
    float omega = DispersionRelation(kLength, spectrumParams.gravity, spectrumParams.depth).x;

    float sinPhase;
    float cosPhase;
    sincos(omega * spectrumParams.time, sinPhase, cosPhase);

    // 시간에 따라 진화하는 높이 스펙트럼을 계산한다.
    // h̃(k, t) = h̃0(k) * exp(i * ω * t) + conj(h̃0(-k)) * exp(-i * ω * t)
    float4 h0 = initialSpectrum.Load(int4(pq, spectrumParams.cascadeIndex, 0));
    float2 exponent = float2(cosPhase, sinPhase);
    float2 h = ComplexMul(h0.xy, exponent) + ComplexMul(h0.zw, float2(exponent.x, -exponent.y));
    float2 ih = float2(-h.y, h.x);

    // 높이 스펙트럼 h 는 켤레 대칭이므로 주파수 반전에서 자기 자신인 DC, Nyquist bin 은 허수부가 0 이다.
    // i * kx 또는 i * ky 를 곱한 파생 스펙트럼도 켤레 대칭을 유지해야 한다.
    // DC 는 waveVector 가 0 이므로 이미 제외되며, centered layout 에서 각 축의 Nyquist 좌표인
    // p == 0 or q == 0 에서는 해당 축의 odd wave vector 성분을 0 으로 처리한다.
    float2 oddWaveVector = float2(
        pq.x == 0u ? 0.0f : waveVector.x,
        pq.y == 0u ? 0.0f : waveVector.y);

    // IFFT 에서 필요한 8개의 실수 field spectrum 을 계산한다.
    spectra.displacementX = -ih * (spectrumParams.choppiness * oddWaveVector.x * invK);
    spectra.displacementY = -ih * (spectrumParams.choppiness * oddWaveVector.y * invK);
    spectra.displacementZ = h;
    spectra.displacementX_dx = h * (spectrumParams.choppiness * waveVector.x * waveVector.x * invK);
    spectra.displacementY_dy = h * (spectrumParams.choppiness * waveVector.y * waveVector.y * invK);
    spectra.displacementX_dy = h * (spectrumParams.choppiness * oddWaveVector.x * oddWaveVector.y * invK);
    spectra.displacementZ_dx = ih * oddWaveVector.x;
    spectra.displacementZ_dy = ih * oddWaveVector.y;
    return spectra;
}

// 두 real sequence 의 complex spectrum A, B 를 하나의 complex spectrum Z 로 합친다.
// Z[k] = A[k] + i * B[k]
// Re(Z[k]) = Re(A[k]) - Im(B[k])
// Im(Z[k]) = Im(A[k]) + Re(B[k])
float2 PackTwoRealSpectra(float2 spectrumA, float2 spectrumB) {
    return float2(spectrumA.x - spectrumB.y, spectrumA.y + spectrumB.x);
}

// 두 real sequence 의 complex spectrum A, B 는 각각 켤레 대칭이므로,
// A[-k] = conj(A[k]), B[-k] = conj(B[k]) 이다.
// 따라서,
// Z[-k] = A[-k] + i * B[-k] = conj(A[k]) + i * conj(B[k]) (Z[k] 는 켤레 대칭이 아님)
// Re(Z[-k]) = Re(A[k]) + Im(B[k])
// Im(Z[-k]) = -Im(A[k]) + Re(B[k])
float2 PackTwoRealMirroredSpectra(float2 spectrumA, float2 spectrumB) {
    return float2(spectrumA.x + spectrumB.y, -spectrumA.y + spectrumB.x);
}

uint GetSpectrumIndex(uint planeIndex, uint binX, uint binY, uint spectrumSize) {
    return planeIndex * spectrumSize * spectrumSize + binY * spectrumSize + binX;
}

// 8 개의 real field spectrum 을 두 개씩 묶어 Z[k] = A[k] + i * B[k] 인 4 개의 complex plane 에 기록한다.
void StoreComplexOceanFieldSpectra(uint binX, uint binY, uint spectrumSize, OceanFieldSpectra spectra) {
    currentSpectrum[GetSpectrumIndex(0u, binX, binY, spectrumSize)] = PackTwoRealSpectra(spectra.displacementX, spectra.displacementY);
    currentSpectrum[GetSpectrumIndex(1u, binX, binY, spectrumSize)] = PackTwoRealSpectra(spectra.displacementZ, spectra.displacementX_dy);
    currentSpectrum[GetSpectrumIndex(2u, binX, binY, spectrumSize)] = PackTwoRealSpectra(spectra.displacementZ_dx, spectra.displacementZ_dy);
    currentSpectrum[GetSpectrumIndex(3u, binX, binY, spectrumSize)] = PackTwoRealSpectra(spectra.displacementX_dx, spectra.displacementY_dy);
}

// 각 field spectrum 의 켤레 대칭 성분 (conj(A[k]), conj(B[k])) 을 이용해서 반대편 주파수 bin (Z[-k]) 의 complex spectrum 을 구성해서 기록한다.
void StoreMirroredComplexOceanFieldSpectra(uint binX, uint binY, uint spectrumSize, OceanFieldSpectra spectra) {
    currentSpectrum[GetSpectrumIndex(0u, binX, binY, spectrumSize)] = PackTwoRealMirroredSpectra(spectra.displacementX, spectra.displacementY);
    currentSpectrum[GetSpectrumIndex(1u, binX, binY, spectrumSize)] = PackTwoRealMirroredSpectra(spectra.displacementZ, spectra.displacementX_dy);
    currentSpectrum[GetSpectrumIndex(2u, binX, binY, spectrumSize)] = PackTwoRealMirroredSpectra(spectra.displacementZ_dx, spectra.displacementZ_dy);
    currentSpectrum[GetSpectrumIndex(3u, binX, binY, spectrumSize)] = PackTwoRealMirroredSpectra(spectra.displacementX_dx, spectra.displacementY_dy);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(OceanTimeSpectrumBlockSizeX, OceanTimeSpectrumBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint spectrumWidth;
    uint spectrumHeight;
    uint spectrumSliceCount;
    initialSpectrum.GetDimensions(spectrumWidth, spectrumHeight, spectrumSliceCount);

    uint binX = dispatchThreadId.x;
    uint binY = dispatchThreadId.y;
    uint spectrumSize = spectrumWidth;
    uint halfSpectrumSize = spectrumSize >> 1u;

    if (binY >= spectrumSize || binX >= halfSpectrumSize) {
        return;
    }

    uint mirrorBinY = binY == 0u ? 0u : spectrumSize - binY;

    // binX = [1, N/2) 는 계산하여 기록하고, 나머지 binX = [N/2 + 1, N - 1] 는 각 실수 field spectrum 의 켤레 대칭을 이용해서 기록한다.
    // x bin 0 과 N/2 는 x 반전 시 자기 자신이므로 별도로 계산한다.
    if (binX > 0u) {
        // N = 8 이라면, x bin 1, 2, 3 을 계산하고, 각 실수 field spectrum 의 켤레 대칭을 이용해 mirror bin 7, 6, 5 를 함께 기록한다.
        OceanFieldSpectra spectra = ComputeOceanFieldSpectra(uint2(binX, binY), spectrumSize);
        uint mirrorBinX = spectrumSize - binX;

        StoreComplexOceanFieldSpectra(binX, binY, spectrumSize, spectra);
        StoreMirroredComplexOceanFieldSpectra(mirrorBinX, mirrorBinY, spectrumSize, spectra);
        return;
    }

    //--------------------------------------------------------------------
    // 여기서부터 binX == 0 thread 가 x bin 0 과 N/2 column 을 함께 처리한다.
    //--------------------------------------------------------------------

    // binY = [1, N/2) 는 계산하여 기록하고, 나머지 binY = [N/2 + 1, N - 1] 는 각 실수 field spectrum 의 켤레 대칭을 이용해서 기록한다.
    // y bin 0 과 N/2 는 y 반전 시 자기 자신이므로 별도로 계산한다.
    if (binY > halfSpectrumSize) {
        return;
    }

    // x bin 0 column 계산
    OceanFieldSpectra spectra0 = ComputeOceanFieldSpectra(uint2(0u, binY), spectrumSize);
    StoreComplexOceanFieldSpectra(0u, binY, spectrumSize, spectra0);

    // x bin N/2 column 계산
    OceanFieldSpectra spectraN2 = ComputeOceanFieldSpectra(uint2(halfSpectrumSize, binY), spectrumSize);
    StoreComplexOceanFieldSpectra(halfSpectrumSize, binY, spectrumSize, spectraN2);

    if (binY != mirrorBinY) {
        StoreMirroredComplexOceanFieldSpectra(0u, mirrorBinY, spectrumSize, spectra0);
        StoreMirroredComplexOceanFieldSpectra(halfSpectrumSize, mirrorBinY, spectrumSize, spectraN2);
    }
}
