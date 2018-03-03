#include "../../Engine/xdcore/precompiled.h"
#pragma hdrstop
#include "CUDA.h"

__global__ void knAdd(float *dst, const float *src0, const float *src1, const int count) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	if (idx < count) {
		dst[idx] = src0[idx] + src1[idx];
	}
}

void CUDA_Add(float *dst, const float *src0, const float *src1, const int count) {
	dim3 grid((unsigned int)ceilf(count / 256.0f));
	dim3 block(256);
	knAdd<<<grid, block>>>(dst, src0, src1, count);	
}

__global__ void knSub(float *dst, const float *src0, const float *src1, const int count) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	if (idx < count) {
		dst[idx] = src0[idx] - src1[idx];
	}
}

void CUDA_Sub(float *dst, const float *src0, const float *src1, const int count) {
	dim3 grid((unsigned int)ceilf(count / 256.0f));
	dim3 block(256);
	knSub<<<grid, block>>>(dst, src0, src1, count);	
}

__global__ void knMul(float *dst, const float *src0, const float *src1, const int count) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	if (idx < count) {
		dst[idx] = src0[idx] * src1[idx];
	}
}

void CUDA_Mul(float *dst, const float *src0, const float *src1, const int count) {
	dim3 grid((unsigned int)ceilf(count / 256.0f));
	dim3 block(256);
	knMul<<<grid, block>>>(dst, src0, src1, count);	
}

__global__ void knDiv(float *dst, const float *src0, const float *src1, const int count) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	if (idx < count) {
		dst[idx] = src0[idx] / src1[idx];
	}
}

void CUDA_Div(float *dst, const float *src0, const float *src1, const int count) {
	dim3 grid((unsigned int)ceilf(count / 256.0f));
	dim3 block(256);
	knDiv<<<grid, block>>>(dst, src0, src1, count);	
}

// FIXME!!
__global__ void knSum(float *dst, const float *src, const int count) {
	int gidx = blockDim.x * blockIdx.x + threadIdx.x;
	int tidx = threadIdx.x;

	__shared__ float partialSum[256];
	partialSum[tidx] = src[gidx];

	for (unsigned int stride = blockDim.x >> 1; stride > 0; stride >>= 1) {
		__syncthreads();
		if (tidx < stride) {
			partialSum[tidx] += partialSum[tidx + stride];
		}
	}

	if (tidx == 0) {
		dst[blockIdx.x] = partialSum[0];
	}
}

void CUDA_Sum(float *dst, const float *src, const int count) {
	dim3 grid(1);
	dim3 block(256);
	knSum<<<grid, block>>>(dst, src, count);	
}

template<int BLOCK_SIZE>
__global__ void knMatrixMultiply(float *dst, const float *src0, const float *src1, const int width) {
#if 1
	// shared memory 에 담을 BLOCK_SIZE x BLOCK_SIZE 크기의 sub matrix
	__shared__ float as[BLOCK_SIZE][BLOCK_SIZE];
	__shared__ float bs[BLOCK_SIZE][BLOCK_SIZE];

	// matrix 의 행, 열 index
	const int row_idx = BLOCK_SIZE * blockIdx.y + threadIdx.y;
	const int col_idx = BLOCK_SIZE * blockIdx.x + threadIdx.x;	

	// thread 별 sub matrix 의 원소들 offset
	const int oa = width * row_idx + threadIdx.x;
	const int ob = width * threadIdx.y + col_idx;

	// 첫번째 sub matrix 의 원소들을 prefetch
	float a = src0[oa];
	float b = src1[ob];

	// block stride 의 최대 크기
	const int end = BLOCK_SIZE * gridDim.x;

	// 결과 값
	float comp = 0.0f;

	for (int stride = BLOCK_SIZE; stride <= end; stride += BLOCK_SIZE) {
		// thread 별로 prefetch 된 원소들을 shared memory 에 저장
		as[threadIdx.y][threadIdx.x] = a;
		bs[threadIdx.y][threadIdx.x] = b;

		// 같은 block 의 shared memory 저장 대기
		__syncthreads();

		// 다음 sub matrix 의 원소들을 prefetch
		a = src0[oa + stride];
		b = src1[ob + width * stride];

		// sub matrix 를 이용해 dot product
		for (int i = 0; i < BLOCK_SIZE; i += 4) {
			comp += as[threadIdx.y][i + 0] * bs[i + 0][threadIdx.x];
			comp += as[threadIdx.y][i + 1] * bs[i + 1][threadIdx.x];
			comp += as[threadIdx.y][i + 2] * bs[i + 2][threadIdx.x];
			comp += as[threadIdx.y][i + 3] * bs[i + 3][threadIdx.x];
		}

		// 다음 sub matrix 의 block 을 위해서 대기
		__syncthreads();
	}

	// global memory 에 결과 값 저장
	dst[width * row_idx + col_idx] = comp;
#else
	const int col_idx = BLOCK_SIZE * blockIdx.x + threadIdx.x;
	const int row_idx = BLOCK_SIZE * blockIdx.y + threadIdx.y;	

	float comp = 0.0f;
	for (int i = 0; i < width; i++) {
		comp += src0[width * row_idx + i] * src1[width * i + col_idx];
	}
	dst[width * row_idx + col_idx] = comp;
#endif
}

template<int BLOCK_SIZE>
__global__ void knMatrixMultiply2(float *dst, const float *src0, const float *src1, const int width) {
	__shared__ float as[BLOCK_SIZE][2*BLOCK_SIZE];
	__shared__ float bs[2*BLOCK_SIZE][BLOCK_SIZE];

	int col_idx = 2*BLOCK_SIZE * blockIdx.x + threadIdx.x;
	int row_idx = BLOCK_SIZE * blockIdx.y + threadIdx.y;

	int oa = width * row_idx + threadIdx.x;
	int ob = width * threadIdx.y + col_idx;

	float a00 = src0[oa];
	float a01 = src0[oa + BLOCK_SIZE];

	float b00 = src1[ob];
	float b01 = src1[ob + BLOCK_SIZE];
	float b10 = src1[ob + width * BLOCK_SIZE];
	float b11 = src1[ob + width * BLOCK_SIZE + BLOCK_SIZE];

	int end = 2*BLOCK_SIZE * gridDim.x;

	float bcomp00 = 0.0f;
	float bcomp01 = 0.0f;
	
	for (int stride = 2*BLOCK_SIZE; stride <= end; stride += 2*BLOCK_SIZE) {
		as[threadIdx.y][threadIdx.x] = a00;
		as[threadIdx.y][threadIdx.x + BLOCK_SIZE] = a01;
		
		bs[threadIdx.y][threadIdx.x] = b00;
		bs[threadIdx.y + BLOCK_SIZE][threadIdx.x] = b10;
		
		__syncthreads();

		a00 = src0[oa + stride];
		a01 = src0[oa + BLOCK_SIZE + stride];
		
		b00 = src1[ob + width * stride];
		b10 = src1[ob + width * (BLOCK_SIZE + stride)];
		
		for (int i = 0; i < 2*BLOCK_SIZE; i += 4) {
			bcomp00 += as[threadIdx.y][i + 0] * bs[i + 0][threadIdx.x];
			bcomp00 += as[threadIdx.y][i + 1] * bs[i + 1][threadIdx.x];
			bcomp00 += as[threadIdx.y][i + 2] * bs[i + 2][threadIdx.x];
			bcomp00 += as[threadIdx.y][i + 3] * bs[i + 3][threadIdx.x];
		}

		__syncthreads();

		bs[threadIdx.y][threadIdx.x] = b01;
		bs[threadIdx.y + BLOCK_SIZE][threadIdx.x] = b11;

		__syncthreads();
				
		b01 = src1[ob + width * stride + BLOCK_SIZE];
		b11 = src1[ob + width * (BLOCK_SIZE + stride) + BLOCK_SIZE];

		for (int i = 0; i < 2*BLOCK_SIZE; i += 4) {
			bcomp01 += as[threadIdx.y][i + 0] * bs[i + 0][threadIdx.x];
			bcomp01 += as[threadIdx.y][i + 1] * bs[i + 1][threadIdx.x];
			bcomp01 += as[threadIdx.y][i + 2] * bs[i + 2][threadIdx.x];
			bcomp01 += as[threadIdx.y][i + 3] * bs[i + 3][threadIdx.x];
		}
		
		__syncthreads();
	}

	dst[width * row_idx + col_idx] = bcomp00;
	dst[width * row_idx + col_idx + BLOCK_SIZE] = bcomp01;	
}

void CUDA_MatrixMultiply(float *dst, const float *src0, const float *src1, const int width) {
#if 1
	if (MyCuda::deviceProp[0].maxThreadsPerBlock >= 1024) {
		dim3 grid((width + 31) / 32, (width + 31) / 32);
		dim3 block(32, 32);
		knMatrixMultiply<32><<<grid, block>>>(dst, src0, src1, width);
	} else {
		dim3 grid((width + 15) / 16, (width + 15) / 16);
		dim3 block(16, 16);
		knMatrixMultiply<16><<<grid, block>>>(dst, src0, src1, width);
	}
#else
	dim3 grid((width + 31) / 32, (width + 15) / 16);
	dim3 block(16, 16);
	knMatrixMultiply2<16><<<grid, block>>>(dst, src0, src1, width);
#endif
}