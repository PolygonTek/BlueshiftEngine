// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BlueshiftEngine.h"

//#define TEST_CUDA       1

#if TEST_CUDA
#include "CUDA.h"
#pragma fenv_access (on)
#endif

#if TEST_CUDA

void CUDA_TestAdd() {
    float bestTimeCUDA;
    const int numComponents = 50000;
    int size = sizeof(float) * numComponents;

    float *dst, *src0, *src1;
    dst = (float *)BE1::Mem_Alloc(size);
    src0 = (float *)BE1::Mem_Alloc(size);
    src1 = (float *)BE1::Mem_Alloc(size);

    RandomFloatArrayInit(src0, numComponents, -100.0f, 100.0f);
    RandomFloatArrayInit(src1, numComponents, -100.0f, 100.0f);

    float *d_dst, *d_src0, *d_src1;
    cudaMalloc((void **)&d_dst, size);
    cudaMalloc((void **)&d_src0, size);
    cudaMalloc((void **)&d_src1, size);

    cudaMemcpyAsync(d_src0, src0, size, cudaMemcpyHostToDevice);
    cudaMemcpyAsync(d_src1, src1, size, cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    bestTimeCUDA = 0;
    for (int i = 0; i < CUDA_TEST_COUNT; i++) {
        float startTime = BE1::Time::FloatTime();
        CUDA_Add(d_dst, d_src0, d_src1, numComponents);
        cudaDeviceSynchronize();
        float endTime = BE1::Time::FloatTime();
        GetBest(startTime, endTime, bestTimeCUDA);
        _CheckCudaError();
    }

    cudaMemcpy(dst, d_dst, size, cudaMemcpyDeviceToHost);

    BE1::Engine::Log(LOG_NORMAL, L"CUDA_Add: %f seconds\n", bestTimeCUDA);

    for (int i = 0; i < numComponents; ++i) {
        float sum = src0[i] + src1[i];
        if (fabs(dst[i] - sum) > 1e-5) {
            BE1::Engine::Log(LOG_NORMAL, L"*ERROR: invalid result\n");
            break;
        }
    }

    cudaFree(d_dst);
    cudaFree(d_src0);
    cudaFree(d_src1);

    BE1::Mem_Free(dst);
    BE1::Mem_Free(src0);
    BE1::Mem_Free(src1);
}

void CUDA_TestSum() {
    float bestTimeCUDA;
    const int numComponents = 256;
    int size = sizeof(float) * numComponents;
    float sum;

    float *src;
    src = (float *)BE1::Mem_Alloc(size);

    RandomFloatArrayInit(src, numComponents, -100.0f, 100.0f);
    for (int i = 0; i < numComponents; i++) {
        src[i] = 1.0f;
    }

    float *d_dst, *d_src;
    cudaMalloc((void **)&d_dst, sizeof(float));
    cudaMalloc((void **)&d_src, size);

    cudaMemcpyAsync(d_src, src, size, cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    bestTimeCUDA = 0;
    for (int i = 0; i < CUDA_TEST_COUNT; i++) {
        float startTime = BE1::Time::FloatTime();
        CUDA_Sum(d_dst, d_src, numComponents);
        cudaDeviceSynchronize();
        float endTime = BE1::Time::FloatTime();
        GetBest(startTime, endTime, bestTimeCUDA);
        _CheckCudaError();
    }

    cudaMemcpy(&sum, d_dst, sizeof(float), cudaMemcpyDeviceToHost);

    BE1::Engine::Log(LOG_NORMAL, L"CUDA_Sum: %f seconds\n", bestTimeCUDA);

    cudaFree(d_src);

    BE1::Mem_Free(src);
}

void CUDA_TestMatrixMultiply() {
    float bestTimeCUDA;
    const int numComponents = 256;
    int size = sizeof(float) * numComponents * numComponents;

    float *dst, *src0, *src1;
    dst = (float *)BE1::Mem_Alloc(size);
    src0 = (float *)BE1::Mem_Alloc(size);
    src1 = (float *)BE1::Mem_Alloc(size);

    RandomFloatArrayInit(src0, numComponents * numComponents, -100.0f, 100.0f);
    RandomFloatArrayInit(src1, numComponents * numComponents, -100.0f, 100.0f);

    float *d_dst, *d_src0, *d_src1;
    cudaMalloc((void **)&d_dst, size);
    cudaMalloc((void **)&d_src0, size);
    cudaMalloc((void **)&d_src1, size);

    cudaMemcpyAsync(d_src0, src0, size, cudaMemcpyHostToDevice);
    cudaMemcpyAsync(d_src1, src1, size, cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    bestTimeCUDA = 0;
    for (int i = 0; i < CUDA_TEST_COUNT; i++) {
        float startTime = BE1::Time::FloatTime();
        CUDA_MatrixMultiply(d_dst, d_src0, d_src1, numComponents);
        cudaDeviceSynchronize();
        float endTime = BE1::Time::FloatTime();
        GetBest(startTime, endTime, bestTimeCUDA);
        _CheckCudaError();
    }

    cudaMemcpy(dst, d_dst, size, cudaMemcpyDeviceToHost);

    BE1::Engine::Log(LOG_NORMAL, L"CUDA_MatrixMultiply: %f seconds\n", bestTimeCUDA);

    cudaFree(d_dst);
    cudaFree(d_src0);
    cudaFree(d_src1);

    BE1::Mem_Free(dst);
    BE1::Mem_Free(src0);
    BE1::Mem_Free(src1);
}

void TestCUDA() {
    BE1::Engine::Log(LOG_NORMAL, L"Testing CUDA kernels..\n");

    CUDA_TestAdd();
    CUDA_TestSum();
    CUDA_TestMatrixMultiply();
}

#endif