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

static void PrintClocksGeneric(const char *string, uint64_t clocks) {
    BE_LOG("generic->%s: %" PRIu64 " clocks\n", string, clocks);
}

static void PrintClocksSIMD(const char *string, uint64_t clocksGeneric, uint64_t clocksSIMD) {
    BE_LOG("   simd->%s: %" PRIu64 " clocks (%.2fx fast)\n", string, clocksSIMD, (float)clocksGeneric / (float)clocksSIMD);
}

static void RandomFloatArrayInit(float *dst, int count, float minimum, float maximum) {
    for (int i = 0; i < count; i++) {
        dst[i] = BE1::Math::Random(minimum, maximum);
    }
}

static void TestAdd() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Add( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Add( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Add( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Add( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestSub() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Sub( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Sub( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Sub( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Sub( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestMul() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Mul( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Mul( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Mul( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Mul( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestDiv() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src0[8192];
    ALIGN_AS32 float src1[8192];
    ALIGN_AS32 float dst[8192];
    float c = 65535;

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Div( float, float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Div( float, float[] )", bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdGeneric->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Div( float[], float[] )", bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        BE1::simdProcessor->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Div( float[], float[] )", bestClocksGeneric, bestClocksSIMD);
}

static void TestSum() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src[8192];
    float sum;

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        sum = BE1::simdGeneric->Sum(src, COUNT_OF(src));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Sum 8192", bestClocksGeneric);

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = rdtsc();
        sum = BE1::simdProcessor->Sum(src, COUNT_OF(src));
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Sum 8192", bestClocksGeneric, bestClocksSIMD);
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

    PrintClocksGeneric("Memcpy 64k Bytes", bestClocksGeneric);

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

    PrintClocksSIMD("Memcpy 64k Bytes", bestClocksGeneric, bestClocksSIMD);

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

    PrintClocksGeneric("Memset 8192 Bytes", bestClocksGeneric);

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

    PrintClocksSIMD("Memset 8192 Bytes", bestClocksGeneric, bestClocksSIMD);

    BE1::Mem_AlignedFree(buffer);
}

static void TestMatrix4x4Multiply() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float matrixA[16 * 1024];
    ALIGN_AS32 float matrixB[16 * 1024];
    ALIGN_AS32 float matrixC[16 * 1024];
    float *matrixAPtr;
    float *matrixBPtr;
    float *matrixCPtr;

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(matrixB, COUNT_OF(matrixB), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;
        matrixCPtr = matrixC;

        uint64_t startClocks = rdtsc();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->Matrix4x4Multiply(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 16;
            matrixBPtr += 16;
            matrixCPtr += 16;
        }
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Matrix4x4Multiply", bestClocksGeneric);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(matrixB, COUNT_OF(matrixB), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;
        matrixCPtr = matrixC;

        uint64_t startClocks = rdtsc();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->Matrix4x4Multiply(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 16;
            matrixBPtr += 16;
            matrixCPtr += 16;
        }
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Matrix4x4Multiply", bestClocksGeneric, bestClocksSIMD);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);	
}

static void TestMatrix4x4Transpose() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float matrixA[16 * 1024];
    ALIGN_AS32 float matrixB[16 * 1024];
    float *matrixAPtr;
    float *matrixBPtr;

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;

        uint64_t startClocks = rdtsc();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->Matrix4x4Transpose(matrixBPtr, matrixAPtr);
            matrixAPtr += 16;
            matrixBPtr += 16;
        }
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Matrix4x4Transpose", bestClocksGeneric);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;

        uint64_t startClocks = rdtsc();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->Matrix4x4Transpose(matrixBPtr, matrixAPtr);
            matrixAPtr += 16;
            matrixBPtr += 16;
        }
        uint64_t endClocks = rdtsc();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Matrix4x4Transpose", bestClocksGeneric, bestClocksSIMD);
}

void TestSIMD() {
    BE_LOG("Testing SIMD processors..\n");

    TestAdd();
    TestSub();
    TestMul();
    TestDiv();
    TestSum();
    TestMemcpy();
    TestMemset();
    TestMatrix4x4Multiply();
    TestMatrix4x4Transpose();
}
