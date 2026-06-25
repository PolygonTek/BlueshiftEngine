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

    GPU FFT

    * Complex FFT:
      - N-point complex spectrum 은 N 개의 float2 로 저장한다.

    * Real FFT spectrum as packed complex:
      - N-point real FFT 결과 X[0..N-1] 는 켤레 대칭이므로 N/2 개의 float2 만 저장한다.
      - packedX[0].x = X[0]      // DC
      - packedX[0].y = X[N/2]    // Nyquist
      - packedX[1..N/2-1] = X[1..N/2-1]

    * Dual-real FFT spectrum as packed complex:
      - 두 real 입력 A, B 를 하나의 complex FFT 로 처리한 뒤, packed real spectrum 두 벌로 분리한다.
      - 한 FFT block 은 N 개의 float2 를 사용한다.
      - packed[0..N/2-1]       = A spectrum
      - packed[N/2..N-1]       = B spectrum
      - A[0].x = A_DC, A[0].y = A_Nyquist, A[1..N/2-1]
      - B[0].x = B_DC, B[0].y = B_Nyquist, B[1..N/2-1]

    * 1D FFT
      - 1D 함수는 fftBlockCount 개의 독립적인 FFT/IFFT 를 수행한다.

    * 2D FFT:
      - Forward 2D FFT 는 한 번만 transpose 하여 transposed 상태로 리턴한다.
      - Inverse 2D FFT 는 입력이 transposed 상태라고 가정한다.
      - 2D 함수는 planeCount 개의 독립적인 2D FFT/IFFT 를 수행한다.
      - plane 은 planeCount 개만큼 버퍼에 연속 배치되며, transpose 도 plane 별로 독립적으로 수행한다.

---------------------------------------------------------------------------------------------------
*/

class GpuFFT {
public:
    void                        Init();
    void                        Shutdown();

                                // 256-point complex buffers -> 256-point complex buffers
    bool                        DispatchFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 512-point complex buffers -> 512-point complex buffers
    bool                        DispatchFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 1024-point complex buffers -> 1024-point complex buffers
    bool                        DispatchFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 256-point complex buffers -> 256-point complex buffers
    bool                        DispatchIFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 512-point complex buffers -> 512-point complex buffers
    bool                        DispatchIFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 1024-point complex buffers -> 1024-point complex buffers
    bool                        DispatchIFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 256 x 256 complex planes -> transposed 256 x 256 complex planes
    bool                        DispatchFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512 x 512 complex planes -> transposed 512 x 512 complex planes
    bool                        DispatchFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 1024 x 1024 complex planes -> transposed 1024 x 1024 complex planes
    bool                        DispatchFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 256 x 256 complex planes -> 256 x 256 complex planes
    bool                        DispatchIFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 512 x 512 complex planes -> 512 x 512 complex planes
    bool                        DispatchIFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 1024 x 1024 complex planes -> 1024 x 1024 complex planes
    bool                        DispatchIFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512-point real buffers -> 256-point packed complex buffers
    bool                        DispatchFFTReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 1024-point real buffers -> 512-point packed complex buffers
    bool                        DispatchFFTReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 256-point packed complex buffers -> 512-point real buffers
    bool                        DispatchIFFTReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 512-point packed complex buffers -> 1024-point real buffers
    bool                        DispatchIFFTReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 512 x 512 real planes -> transposed 512 x 256 packed complex planes
    bool                        DispatchFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 1024 x 1024 real planes -> transposed 1024 x 512 packed complex planes
    bool                        DispatchFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 512 x 256 packed complex planes -> 512 x 512 real planes
    bool                        DispatchIFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // transposed 1024 x 512 packed complex planes -> 1024 x 1024 real planes
    bool                        DispatchIFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount);

                                // 512-point dual-real buffers -> 512-point packed complex buffers
    bool                        DispatchFFTDualReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // 1024-point dual-real buffers -> 1024-point packed complex buffers
    bool                        DispatchFFTDualReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 512-point packed complex buffers -> 512-point dual-real buffers
    bool                        DispatchIFFTDualReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

                                // inverse 1024-point packed complex buffers -> 1024-point dual-real buffers
    bool                        DispatchIFFTDualReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount);

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
    RHI::Buffer *               CreateRealFFTTwiddleBuffer(uint32_t fftSize);

    bool                        DispatchFFTComplexStage(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::ConstantBuffer *fftConstantBuffer, uint32_t stageIndex);
    bool                        DispatchFFTComplex256(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchFFTComplex512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchFFTComplex1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchFFTComplexToOutput(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, uint32_t regularStageCount, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, float sign, float scale);
    bool                        DispatchIFFTComplex256(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTComplex512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTComplex1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTComplexToOutput(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, uint32_t regularStageCount, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, float sign, float scale);

    bool                        DispatchFFTReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchFFTReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);
    bool                        DispatchIFFTReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);

    bool                        DispatchFFTDualReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchFFTDualReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTDualReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);
    bool                        DispatchIFFTDualReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);

    RHI::Buffer *               realFFTTwiddle512Buffer = nullptr;
    RHI::Buffer *               realFFTTwiddle1024Buffer = nullptr;
};
