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
#include "TestSIMD.h"

#define TEST_COUNT			4096
#define CUDA_TEST_COUNT		30

#define GetBest(start, end, best) \
    if (!best || end - start < best) { \
        best = end - start; \
    }

static void PrintClocksGeneric(const wchar_t *string, uint64_t clocks) {
    BE_LOG(L"generic->%ls: %" PRIu64 " clocks\n", string, clocks);
}

static void PrintClocksSIMD(const wchar_t *string, uint64_t clocksGeneric, uint64_t clocksSIMD) {
    BE_LOG(L"   simd->%ls: %" PRIu64 " clocks (%.2fx fast)\n", string, clocksSIMD, (float)clocksGeneric / (float)clocksSIMD);
}

static void RandomFloatArrayInit(float *dst, int count, float minimum, float maximum) {
    for (int i = 0; i < count; i++) {
        dst[i] = BE1::Math::Random(minimum, maximum);
    }
}

static void TestAdd() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float src0[1024]);
    ALIGN32(float src1[1024]);
    ALIGN32(float dst[1024]);
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Add( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Add( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Add( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Add( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestSub() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float src0[1024]);
    ALIGN32(float src1[1024]);
    ALIGN32(float dst[1024]);
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Sub( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Sub( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Sub( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Sub( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestMul() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float src0[1024]);
    ALIGN32(float src1[1024]);
    ALIGN32(float dst[1024]);
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Mul( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Mul( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Mul( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Mul( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestDiv() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float src0[1024]);
    ALIGN32(float src1[1024]);
    ALIGN32(float dst[1024]);
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Div( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Div( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Div( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Div( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestSum() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float src[1024]);
    float sum;

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        sum = BE1::simdGeneric->Sum(src, COUNT_OF(src));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Sum 1024", bestClocksGeneric);

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        sum = BE1::simdProcessor->Sum(src, COUNT_OF(src));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Sum 1024", bestClocksGeneric, bestClocksSIMD);
}

static void TestMemcpy() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 1024 * 64;
    unsigned char *bufferSrc = (unsigned char *)BE1::Mem_Alloc32(bufferSize);
    unsigned char *bufferDst = (unsigned char *)BE1::Mem_Alloc32(bufferSize);

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            bufferSrc[j] = rand() % 256;
        }

        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Memcpy 64k Bytes", bestClocksGeneric);

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            bufferSrc[j] = rand() % 256;
        }

        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Memcpy 64k Bytes", bestClocksGeneric, bestClocksSIMD);

    BE1::Mem_AlignedFree(bufferSrc);
    BE1::Mem_AlignedFree(bufferDst);
}

static void TestMemset() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 0x2000;
    unsigned char *buffer = (unsigned char *)BE1::Mem_Alloc32(bufferSize);

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            buffer[j] = rand() % 256;
        }

        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Memset(buffer, 0, bufferSize);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"Memset 8192 Bytes", bestClocksGeneric);

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            buffer[j] = rand() % 256;
        }

        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Memset(buffer, 0, bufferSize);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"Memset 8192 Bytes", bestClocksGeneric, bestClocksSIMD);

    BE1::Mem_AlignedFree(buffer);
}

static void TestMatrixMultiply() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float matrixA[16]);
    ALIGN32(float matrixB[16]);
    ALIGN32(float matrixC[16]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(matrixB, COUNT_OF(matrixB), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->MatrixMultiply(matrixC, matrixA, matrixB);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"MatrixMultiply", bestClocksGeneric);
    //BE_LOG(L"  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", 
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->MatrixMultiply(matrixC, matrixA, matrixB);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"MatrixMultiply", bestClocksGeneric, bestClocksSIMD);
    //BE_LOG(L"  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);	
}

static void TestMatrixTranspose() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN32(float matrixA[16]);
    ALIGN32(float matrixB[16]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->MatrixTranspose(matrixB, matrixA);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(L"MatrixTranspose", bestClocksGeneric);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->MatrixTranspose(matrixB, matrixA);
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(L"MatrixTranspose", bestClocksGeneric, bestClocksSIMD);
}

void TestSIMD() {
    BE_LOG(L"Testing SIMD processors..\n");

    TestAdd();
    TestSub();
    TestMul();
    TestDiv();
    TestSum();
    TestMemcpy();
    TestMemset();
    TestMatrixMultiply();
    TestMatrixTranspose();
}
