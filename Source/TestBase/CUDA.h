#ifndef _CUDA_H_INCLUDED_
#define _CUDA_H_INCLUDED_

/*
====================================================================================================

	CUDA
	
====================================================================================================
*/

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>

class MyCuda {
public:
	static bool				Init();
	static void				Shutdown();

	static int				driverVersion;
	static int				runtimeVersion;

	static cudaDeviceProp	deviceProp[2];
};

#define _CheckCudaError() CheckCudaError(__FILE__, __LINE__)
void CheckCudaError(const char *file, const int line);

#include "cuda_kernels.cuh"

#endif // _CUDA_H_INCLUDED_