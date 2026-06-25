// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Precompiled.h"
#include "RHI/RHI.h"

class RenderFrameData;

/*
---------------------------------------------------------------------------------------------------

    FFT 2D

    * FFT 함수는 한번만 transpose 하여 transposed 상태로 리턴한다. IFFT 함수에서는 입력으로 transposed 상태를 가정한다.
    * plane 은 planeCount 개만큼 버퍼에 연속으로 배치되며, transpose 도 plane 별로 독립적으로 수행한다.
    * real FFT packed complex 레이아웃: Z[0].re = DC, Z[0].im = Nyquist, 나머지는 켤레 대칭이므로 절반인 Z[1..N/2-1] 만 저장하면 된다.
    * dual-real plane 은 float2 한 plane 에 두 real signal 을 a/b 로 pack 한 것이다.
    * dual-real FFT 결과는 transpose 된 같은 plane 안에 두 packed spectrum 을 저장한다. (A[0], A[1], .., A[N/2-1], B[0], B[1], .., B[N/2-1])

---------------------------------------------------------------------------------------------------
*/

class FFT2D {
public:
    void                        Init();
    void                        Shutdown();

                                // 256 x 256 complex planes -> transposed 256 x 256 complex planes
    bool                        DispatchFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 256 x 256 complex planes -> 256 x 256 complex planes
    bool                        DispatchIFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512 x 512 complex planes -> transposed 512 x 512 complex planes
    bool                        DispatchFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 512 x 512 complex planes -> 512 x 512 complex planes
    bool                        DispatchIFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 1024 x 1024 complex planes -> transposed 1024 x 1024 complex planes
    bool                        DispatchFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 1024 x 1024 complex planes -> 1024 x 1024 complex planes
    bool                        DispatchIFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512 x 512 real planes -> transposed 512 x 256 packed complex planes
    bool                        DispatchFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 1024 x 1024 real planes -> transposed 1024 x 512 packed complex planes
    bool                        DispatchFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 512 x 256 packed complex planes -> 512 x 512 real planes
    bool                        DispatchIFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 1024 x 512 packed complex planes -> 1024 x 1024 real planes
    bool                        DispatchIFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512 x 512 dual-real planes -> transposed 512 x 512 packed complex planes
    bool                        DispatchFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 1024 x 1024 dual-real planes -> transposed 1024 x 1024 packed complex planes
    bool                        DispatchFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 512 x 512 packed complex planes -> 512 x 512 dual-real planes
    bool                        DispatchIFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 1024 x 1024 packed complex planes -> 1024 x 1024 dual-real planes
    bool                        DispatchIFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // fftSize x fftSize complex spectra -> fftSize x fftSize complex spectra
    bool                        DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

                                // transposed fftSize x fftSize/2 packed real spectra -> transposed fftSize x fftSize/2 packed real spectra
    bool                        DispatchMultiplyPackedRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

                                // transposed fftSize x fftSize packed dual-real spectra -> transposed fftSize x fftSize packed dual-real spectra
    bool                        DispatchMultiplyPackedDualRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

                                // complex to complex
    RHI::PipelineState *        fftComplex256PSO = nullptr;
    RHI::PipelineState *        fftComplex512PSO = nullptr;
    RHI::PipelineState *        fftComplex1024PSO = nullptr;
    RHI::PipelineState *        fftComplex1024LastStagePSO = nullptr;

                                // real to packed complex
    RHI::PipelineState *        fftReal512FirstStagePSO = nullptr;
    RHI::PipelineState *        fftReal512LastStagePSO = nullptr;
    RHI::PipelineState *        fftReal1024FirstStagePSO = nullptr;
    RHI::PipelineState *        fftReal1024LastStagePSO = nullptr;

                                // packed complex to real
    RHI::PipelineState *        ifftReal512FirstStagePSO = nullptr;
    RHI::PipelineState *        ifftReal512LastStagePSO = nullptr;
    RHI::PipelineState *        ifftReal1024FirstStagePSO = nullptr;
    RHI::PipelineState *        ifftReal1024LastStagePSO = nullptr;

                                // dual real to 2 packed complex
    RHI::PipelineState *        fftDualReal512FirstStagePSO = nullptr;
    RHI::PipelineState *        fftDualReal512LastStagePSO = nullptr;
    RHI::PipelineState *        fftDualReal1024FirstStagePSO = nullptr;
    RHI::PipelineState *        fftDualReal1024LastStagePSO = nullptr;

                                // 2 packed complex to dual real
    RHI::PipelineState *        ifftDualReal512FirstStagePSO = nullptr;
    RHI::PipelineState *        ifftDualReal512LastStagePSO = nullptr;
    RHI::PipelineState *        ifftDualReal1024FirstStagePSO = nullptr;
    RHI::PipelineState *        ifftDualReal1024LastStagePSO = nullptr;

                                // packed spectrum ops
    RHI::PipelineState *        multiplyComplexSpectrum2DPSO = nullptr;
    RHI::PipelineState *        multiplyPackedRealSpectrum2DPSO = nullptr;
    RHI::PipelineState *        multiplyPackedDualRealSpectrum2DPSO = nullptr;

                                // transpose complex
    RHI::PipelineState *        transposeComplex256PSO = nullptr;
    RHI::PipelineState *        transposeComplex256x512PSO = nullptr;
    RHI::PipelineState *        transposeComplex512x256PSO = nullptr;
    RHI::PipelineState *        transposeComplex512x1024PSO = nullptr;
    RHI::PipelineState *        transposeComplex1024x512PSO = nullptr;
    RHI::PipelineState *        transposeComplex512PSO = nullptr;
    RHI::PipelineState *        transposeComplex1024PSO = nullptr;

private:
    struct FFTComplex2DDispatchDesc {
        uint32_t                fftSize;
        uint32_t                regularStageCount;
        RHI::PipelineState *    regularStagePSO;
        RHI::PipelineState *    lastStagePSO;
        RHI::PipelineState *    transposePSO;
    };

    RHI::Buffer *               CreateRealFFTTwiddleBuffer(uint32_t fftSize);
    bool                        DispatchFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount, const FFTComplex2DDispatchDesc &desc);
    bool                        DispatchIFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount, const FFTComplex2DDispatchDesc &desc);

    RHI::Buffer *               realFFTTwiddle512Buffer = nullptr;
    RHI::Buffer *               realFFTTwiddle1024Buffer = nullptr;
};
