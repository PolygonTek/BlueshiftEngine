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

#define TEST_COUNT          4096
#define CUDA_TEST_COUNT     30

#define GetBest(start, end, best) \
    if (!best || end - start < best) { \
        best = end - start; \
    }

static void PrintClocksGeneric(const char *string, uint64_t clocks) {
    BE_LOG("Generic->%s: %" PRIu64 " clocks\n", string, clocks);
}

static void PrintClocksSIMD(const char *string, uint64_t clocksGeneric, uint64_t clocksSIMD) {
    BE_LOG("   SIMD->%s: %" PRIu64 " clocks (%.2fx fast)\n", string, clocksSIMD, (float)clocksGeneric / (float)clocksSIMD);
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
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Add( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Add(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Add( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Add( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Add(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Add( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
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
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Sub( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Sub(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Sub( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Sub( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Sub(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Sub( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
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
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Mul( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Mul(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Mul( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Mul( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Mul(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Mul( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
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
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Div( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Div(dst, c, src0, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Div( float, float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Div( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric);

    RandomFloatArrayInit(src0, COUNT_OF(dst), -100.0f, 100.0f);
    RandomFloatArrayInit(src1, COUNT_OF(dst), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Div(dst, src0, src1, COUNT_OF(dst));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Div( float[], float[], %i )", COUNT_OF(dst)), bestClocksGeneric, bestClocksSIMD);
}

static void TestSum() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float src[8192];
    float sum;

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        sum = BE1::simdGeneric->Sum(src, COUNT_OF(src));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("Sum 8192", bestClocksGeneric);

    RandomFloatArrayInit(src, COUNT_OF(src), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        sum = BE1::simdProcessor->Sum(src, COUNT_OF(src));
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("Sum 8192", bestClocksGeneric, bestClocksSIMD);
}

static void TestMemcpy() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 1024 * 16 + 255;
    unsigned char *bufferSrc = (unsigned char *)BE1::Mem_Alloc16(bufferSize);
    unsigned char *bufferDst = (unsigned char *)BE1::Mem_Alloc16(bufferSize);

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            bufferSrc[j] = rand() % 256;
        }

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Memcpy %i Bytes", bufferSize), bestClocksGeneric);

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            bufferSrc[j] = rand() % 256;
        }

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Memcpy(bufferDst, bufferSrc, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Memcpy %i Bytes", bufferSize), bestClocksGeneric, bestClocksSIMD);

    BE1::Mem_AlignedFree(bufferSrc);
    BE1::Mem_AlignedFree(bufferDst);
}

static void TestMemset() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    int bufferSize = 1024 * 32 + 255;
    unsigned char *buffer = (unsigned char *)BE1::Mem_Alloc16(bufferSize);

    bestClocksGeneric = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            buffer[j] = rand() % 256;
        }

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->Memset(buffer, 255, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric(BE1::va("Memset %i Bytes", bufferSize), bestClocksGeneric);

    bestClocksSIMD = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < bufferSize; j++) {
            buffer[j] = rand() % 256;
        }

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->Memset(buffer, 255, bufferSize);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD(BE1::va("Memset %i Bytes", bufferSize), bestClocksGeneric, bestClocksSIMD);

    BE1::Mem_AlignedFree(buffer);
}

static void TestMulMat3x4RM() {
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

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->MulMat3x4RM(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 4 * 3;
            matrixBPtr += 4 * 3;
            matrixCPtr += 4 * 3;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("MulMat3x4RM", bestClocksGeneric);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(matrixB, COUNT_OF(matrixB), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;
        matrixCPtr = matrixC;

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->MulMat3x4RM(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 4 * 3;
            matrixBPtr += 4 * 3;
            matrixCPtr += 4 * 3;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("MulMat3x4RM", bestClocksGeneric, bestClocksSIMD);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);	
}

static void TestMulMat4x4RM() {
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

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->MulMat4x4RM(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 4 * 4;
            matrixBPtr += 4 * 4;
            matrixCPtr += 4 * 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("MulMat4x4RM", bestClocksGeneric);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(matrixB, COUNT_OF(matrixB), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;
        matrixCPtr = matrixC;

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->MulMat4x4RM(matrixCPtr, matrixAPtr, matrixBPtr);
            matrixAPtr += 4 * 4;
            matrixBPtr += 4 * 4;
            matrixCPtr += 4 * 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("MulMat4x4RM", bestClocksGeneric, bestClocksSIMD);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);	
}

static void TestMulMat4x4RMVec4() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 float matrixA[16 * 1024];
    ALIGN_AS32 float vectorB[4 * 1024];
    ALIGN_AS32 float vectorC[4 * 1024];
    float *matrixAPtr;
    float *vectorBPtr;
    float *vectorCPtr;

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(vectorB, COUNT_OF(vectorB), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        vectorBPtr = vectorB;
        vectorCPtr = vectorC;

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->MulMat4x4RMVec4(vectorCPtr, matrixAPtr, vectorBPtr);
            matrixAPtr += 4 * 4;
            vectorBPtr += 4;
            vectorCPtr += 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("MulMat4x4RMVec4", bestClocksGeneric);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);
    RandomFloatArrayInit(vectorB, COUNT_OF(vectorB), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        vectorBPtr = vectorB;
        vectorCPtr = vectorC;

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->MulMat4x4RMVec4(vectorCPtr, matrixAPtr, vectorBPtr);
            matrixAPtr += 4 * 4;
            vectorBPtr += 4;
            vectorCPtr += 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("MulMat4x4RMVec4", bestClocksGeneric, bestClocksSIMD);
    //BE_LOG("  Result: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
    //  matrixC[0], matrixC[1], matrixC[2], matrixC[3], matrixC[4], matrixC[5], matrixC[6], matrixC[7], matrixC[8], matrixC[9], matrixC[10], matrixC[11], matrixC[12], matrixC[13], matrixC[14], matrixC[15]);	
}

static void TestTransposeMat4x4() {
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

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdGeneric->TransposeMat4x4(matrixBPtr, matrixAPtr);
            matrixAPtr += 4 * 4;
            matrixBPtr += 4 * 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("TransposeMat4x4", bestClocksGeneric);

    RandomFloatArrayInit(matrixA, COUNT_OF(matrixA), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        matrixAPtr = matrixA;
        matrixBPtr = matrixB;

        uint64_t startClocks = BE1::PlatformTime::Cycles();
        for (int j = 0; j < 1024; j++) {
            BE1::simdProcessor->TransposeMat4x4(matrixBPtr, matrixAPtr);
            matrixAPtr += 4 * 4;
            matrixBPtr += 4 * 4;
        }
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("TransposeMat4x4", bestClocksGeneric, bestClocksSIMD);
}

static void TestTransformJoints() {
    uint64_t bestClocksGeneric;
    uint64_t bestClocksSIMD;
    ALIGN_AS32 BE1::Mat3x4 jointMatrix[1024];
    int parents[1024];

    parents[0] = -1;
    for (int i = 1; i < COUNT_OF(parents); i++) {
        parents[i] = RANDOM_INT(0, i - 1);
    }

    RandomFloatArrayInit((float *)jointMatrix, 12 * COUNT_OF(jointMatrix), -100.0f, 100.0f);

    bestClocksGeneric = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdGeneric->TransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksGeneric);
    }

    PrintClocksGeneric("TransformJoints", bestClocksGeneric);

    RandomFloatArrayInit((float *)jointMatrix, 12 * COUNT_OF(jointMatrix), -100.0f, 100.0f);

    bestClocksSIMD = 0;
    for (int i = 0; i < TEST_COUNT; i++) {
        uint64_t startClocks = BE1::PlatformTime::Cycles();
        BE1::simdProcessor->TransformJoints(jointMatrix, parents, 0, COUNT_OF(jointMatrix) - 1);
        uint64_t endClocks = BE1::PlatformTime::Cycles();
        GetBest(startClocks, endClocks, bestClocksSIMD);
    }

    PrintClocksSIMD("TransformJoints", bestClocksGeneric, bestClocksSIMD);
}

void TestSIMD() {
    BE_LOG("Testing SIMD processors..\n");

    TestAdd();
    TestSub();
    TestMul();
    TestDiv();
    TestSum();
    TestMulMat3x4RM();
    TestMulMat4x4RM();
    TestMulMat4x4RMVec4();
    TestTransposeMat4x4();
    TestMemcpy();
    TestMemset();
    TestTransformJoints();
}
