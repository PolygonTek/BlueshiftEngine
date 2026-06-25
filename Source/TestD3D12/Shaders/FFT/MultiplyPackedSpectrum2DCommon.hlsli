#ifndef __MULTIPLY_PACKED_SPECTRUM_2D_COMMON_HLSLI__
#define __MULTIPLY_PACKED_SPECTRUM_2D_COMMON_HLSLI__

#include "FFTCommon.hlsli"

// rowBase 가 가리키는 row 에는 real spectrum 의 DC[0..N-1], Nyquist[0..N-1] 성분들을 packed complex 로 보고 FFT 정변환한 데이터가 들어있다.
// z[y] = DC[y] + i * Nyquist[y]
// Z[k] = FFT(z)[k]
//      = FFT(DC)[k] + i * FFT(Nyquist)[k] = A[k] + i * B[k]
float2 PackDCNyquistRowProduct(uint rowBase, uint columnIndex, uint fftSize) {
    const uint index = rowBase + columnIndex;
    const float2 Z1 = lhsSpectrumBuffer[index];
    const float2 Z2 = rhsSpectrumBuffer[index];

    // Z[0], Z[N/2] 에서는 A 와 B 가 각각 실수값이므로, 성분별로 곱하기만 하면 된다.
    if (columnIndex == 0 || columnIndex == (fftSize >> 1u)) {
        return Z1 * Z2;
    }

    // Z[k] = A[k] + iB[k] 로 packed 된 real spectrum 을 A 와 B 로 분리한다.
    // A[k] =      (Z[k] + conj(Z[N-k])) / 2
    // B[k] = -i * (Z[k] - conj(Z[N-k])) / 2
    const uint mirrorIndex = rowBase + fftSize - columnIndex;
    const float2 Z1Mirror = lhsSpectrumBuffer[mirrorIndex];
    const float2 Z2Mirror = rhsSpectrumBuffer[mirrorIndex];

    const float2 Z1MirrorConj = ComplexConj(Z1Mirror);
    const float2 Z1A = 0.5 * (Z1 + Z1MirrorConj);
    const float2 Z1Diff = Z1 - Z1MirrorConj;
    const float2 Z1B = 0.5 * float2(Z1Diff.y, -Z1Diff.x);

    const float2 Z2MirrorConj = ComplexConj(Z2Mirror);
    const float2 Z2A = 0.5 * (Z2 + Z2MirrorConj);
    const float2 Z2Diff = Z2 - Z2MirrorConj;
    const float2 Z2B = 0.5 * float2(Z2Diff.y, -Z2Diff.x);

    // A 는 A 끼리 곱하고, B 는 B 끼리 곱한다.
    const float2 outputA = ComplexMul(Z1A, Z2A);
    const float2 outputB = ComplexMul(Z1B, Z2B);

    // 곱한 A, B 를 다시 Z[k] = A[k] + iB[k] 형태로 packing 한다.
    return outputA + float2(-outputB.y, outputB.x);
}

#endif
