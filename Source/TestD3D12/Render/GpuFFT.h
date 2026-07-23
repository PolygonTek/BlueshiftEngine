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

class ShaderPipeline;
class ShaderTemplate;

/*
---------------------------------------------------------------------------------------------------

    GPU FFT

    * Complex FFT:
      - N-point complex spectrum 은 N 개의 float2 로 저장한다.

    * Real FFT spectrum as packed complex:
      - N-point real FFT 결과 X[0..N-1] 는 켤레 대칭이므로 N/2 개의 float2 만 저장한다.
      - packedX[0].x = X[0]   (DC)
      - packedX[0].y = X[N/2] (Nyquist)
      - packedX[1..N/2-1] = X[1..N/2-1]

    * Dual-real FFT spectrum as packed complex:
      - 두 real 입력 A, B 를 하나의 complex FFT 로 처리한 뒤, packed real spectrum 두 벌로 분리한다.
      - 한 FFT block 은 N 개의 float2 를 사용한다.
      - packed[0..N/2-1] = A spectrum
      - packed[N/2..N-1] = B spectrum
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
    enum class FFTSize : uint32_t {
        FFT256,
        FFT512,
        FFT1024,
        FFT2048,
        Count
    };

    void                        Init();
    void                        Shutdown();

    void                        UpdatePipelineBuildState();

    bool                        ArePipelinesReady() const { return pipelinesReady; }
    bool                        HasPipelineBuildFailed() const { return pipelineBuildFailed; }

    static bool                 GetGpuFFTSize(uint32_t fftSizeValue, GpuFFT::FFTSize &fftSize);

                                // fftSize-point complex buffers -> fftSize-point complex buffers (256/512/1024/2048)
    bool                        DispatchFFTComplex1D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // inverse fftSize-point complex buffers -> fftSize-point complex buffers (256/512/1024/2048)
    bool                        DispatchIFFTComplex1D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // fftSize x fftSize complex planes -> transposed fftSize x fftSize complex planes (256/512/1024/2048)
    bool                        DispatchFFTComplex2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // transposed fftSize x fftSize complex planes -> fftSize x fftSize complex planes (256/512/1024/2048)
    bool                        DispatchIFFTComplex2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // fftSize-point real buffers -> fftSize/2-point packed complex buffers (512/1024/2048)
    bool                        DispatchFFTReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // inverse fftSize/2-point packed complex buffers -> fftSize-point real buffers (512/1024/2048)
    bool                        DispatchIFFTReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // fftSize x fftSize real planes -> transposed fftSize x fftSize/2 packed complex planes (512/1024/2048)
    bool                        DispatchFFTReal2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // transposed fftSize x fftSize/2 packed complex planes -> fftSize x fftSize real planes (512/1024/2048)
    bool                        DispatchIFFTReal2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // fftSize-point dual-real buffers -> fftSize-point packed complex buffers (256/512/1024/2048)
    bool                        DispatchFFTDualReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // inverse fftSize-point packed complex buffers -> fftSize-point dual-real buffers (256/512/1024/2048)
    bool                        DispatchIFFTDualReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount);

                                // fftSize x fftSize dual-real planes -> transposed fftSize x fftSize packed complex planes (256/512/1024/2048)
    bool                        DispatchFFTDualReal2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // transposed fftSize x fftSize packed complex planes -> fftSize x fftSize dual-real planes (256/512/1024/2048)
    bool                        DispatchIFFTDualReal2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount);

                                // fftSize x fftSize complex spectra -> fftSize x fftSize complex spectra
    bool                        DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

                                // transposed fftSize x fftSize/2 packed real spectra -> transposed fftSize x fftSize/2 packed real spectra
    bool                        DispatchMultiplyPackedRealSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

                                // transposed fftSize x fftSize packed dual-real spectra -> transposed fftSize x fftSize packed dual-real spectra
    bool                        DispatchMultiplyPackedDualRealSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale);

private:
    RHI::Buffer *               CreateRealFFTTwiddleBuffer(uint32_t fftSize);

    bool                        DispatchFFTComplexFused(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState outputBufferState, float sign, float scale);
    bool                        DispatchFFTRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *twiddleBuffer, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *twiddleBuffer, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);
    bool                        DispatchFFTDualRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer);
    bool                        DispatchIFFTDualRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState);

                                // complex to complex shader templates
    SharedPtr<ShaderTemplate>   fftComplex256ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftComplex512ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftComplex1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftComplex2048ShaderTemplate;

                                // real to packed complex shader templates
    SharedPtr<ShaderTemplate>   fftReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftReal1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftReal2048ShaderTemplate;

                                // packed complex to real shader templates
    SharedPtr<ShaderTemplate>   ifftReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>   ifftReal1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   ifftReal2048ShaderTemplate;

                                // dual real to 2 packed complex shader templates
    SharedPtr<ShaderTemplate>   fftDualReal256ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftDualReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftDualReal1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   fftDualReal2048ShaderTemplate;

                                // 2 packed complex to dual real shader templates
    SharedPtr<ShaderTemplate>   ifftDualReal256ShaderTemplate;
    SharedPtr<ShaderTemplate>   ifftDualReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>   ifftDualReal1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   ifftDualReal2048ShaderTemplate;

                                // packed spectrum shader templates
    SharedPtr<ShaderTemplate>   multiplyComplexSpectrum2DShaderTemplate;
    SharedPtr<ShaderTemplate>   multiplyPackedRealSpectrum2DShaderTemplate;
    SharedPtr<ShaderTemplate>   multiplyPackedDualRealSpectrum2DShaderTemplate;

                                // transpose complex shader templates
    SharedPtr<ShaderTemplate>   transposeComplex256ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex512ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex2048ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex256x512ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex512x256ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex512x1024ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex1024x512ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex1024x2048ShaderTemplate;
    SharedPtr<ShaderTemplate>   transposeComplex2048x1024ShaderTemplate;

                                // complex to complex (fused)
    SharedPtr<ShaderPipeline>   fftComplex256Pipeline;
    SharedPtr<ShaderPipeline>   fftComplex512Pipeline;
    SharedPtr<ShaderPipeline>   fftComplex1024Pipeline;
    SharedPtr<ShaderPipeline>   fftComplex2048Pipeline;

                                // real to packed complex (fused)
    SharedPtr<ShaderPipeline>   fftReal512Pipeline;
    SharedPtr<ShaderPipeline>   fftReal1024Pipeline;
    SharedPtr<ShaderPipeline>   fftReal2048Pipeline;

                                // packed complex to real (fused)
    SharedPtr<ShaderPipeline>   ifftReal512Pipeline;
    SharedPtr<ShaderPipeline>   ifftReal1024Pipeline;
    SharedPtr<ShaderPipeline>   ifftReal2048Pipeline;

                                // dual real to 2 packed complex (fused)
    SharedPtr<ShaderPipeline>   fftDualReal256Pipeline;
    SharedPtr<ShaderPipeline>   fftDualReal512Pipeline;
    SharedPtr<ShaderPipeline>   fftDualReal1024Pipeline;
    SharedPtr<ShaderPipeline>   fftDualReal2048Pipeline;

                                // 2 packed complex to dual real (fused)
    SharedPtr<ShaderPipeline>   ifftDualReal256Pipeline;
    SharedPtr<ShaderPipeline>   ifftDualReal512Pipeline;
    SharedPtr<ShaderPipeline>   ifftDualReal1024Pipeline;
    SharedPtr<ShaderPipeline>   ifftDualReal2048Pipeline;

                                // packed spectrum ops
    SharedPtr<ShaderPipeline>   multiplyComplexSpectrum2DPipeline;
    SharedPtr<ShaderPipeline>   multiplyPackedRealSpectrum2DPipeline;
    SharedPtr<ShaderPipeline>   multiplyPackedDualRealSpectrum2DPipeline;

                                // transpose complex
    SharedPtr<ShaderPipeline>   transposeComplex256Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex512Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex1024Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex2048Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex256x512Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex512x256Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex512x1024Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex1024x512Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex1024x2048Pipeline;
    SharedPtr<ShaderPipeline>   transposeComplex2048x1024Pipeline;

                                // twiddle buffer for real FFT/IFFT
    RHI::Buffer *               fftRealTwiddle512Buffer = nullptr;
    RHI::Buffer *               fftRealTwiddle1024Buffer = nullptr;
    RHI::Buffer *               fftRealTwiddle2048Buffer = nullptr;

    bool                        pipelinesReady = false;
    bool                        pipelineBuildFailed = false;
};
