#include "../../Engine/xdcore/precompiled.h"
#pragma hdrstop
#include "cuda.h"
#include "cuda_kernels.cuh"

int MyCuda::driverVersion = 0;
int	MyCuda::runtimeVersion = 0;
cudaDeviceProp MyCuda::deviceProp[2] = { { 0, }, };

void CheckCudaError(const char *file, const int line) {
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        xd::Core::Error(ERR_FATAL, L"%hs(%i) : CUDA error (%d) : %hs.\n", file, line, (int)err, cudaGetErrorString(err));
        exit(-1);
    }
}

static int ConvertSMVer2Cores(int major, int minor) {
	// Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
	typedef struct {
		int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
		int Cores;
	} sSMtoCores;

	sSMtoCores nGpuArchCoresPerSM[] = { 
		{ 0x10,  8 }, // Tesla Generation (SM 1.0) G80 class
		{ 0x11,  8 }, // Tesla Generation (SM 1.1) G8x class
		{ 0x12,  8 }, // Tesla Generation (SM 1.2) G9x class
		{ 0x13,  8 }, // Tesla Generation (SM 1.3) GT200 class
		{ 0x20, 32 }, // Fermi Generation (SM 2.0) GF100 class
		{ 0x21, 48 }, // Fermi Generation (SM 2.1) GF10x class
		{ 0x30, 192}, // Fermi Generation (SM 3.0) GK10x class
		{   -1, -1 }
	};

	int index = 0;
	while (nGpuArchCoresPerSM[index].SM != -1) {
		if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor)) {
			return nGpuArchCoresPerSM[index].Cores;
		}
		index++;
	}

	xd::Core::Log(LOG_WARNING, L"ConvertSMVert2Cores SM %d.%d is undefined (please update to the latest SDK)!\n", major, minor);
	return -1;
}

bool MyCuda::Init() {
	int deviceCount;

	cudaGetDeviceCount(&deviceCount);
	if (deviceCount == 0) {
		xd::Core::Log(LOG_NORMAL, L"MyCuda::Init: no devices supporting CUDA.\n");
		return false;
	}

	cudaDriverGetVersion(&driverVersion);
	cudaRuntimeGetVersion(&runtimeVersion);

	xd::Core::Log(LOG_DEVELOPER, L"CUDA Driver Version: %d.%d\n", driverVersion/1000, (driverVersion%100)/10);
	xd::Core::Log(LOG_DEVELOPER, L"CUDA Runtime Version: %d.%d\n", runtimeVersion/1000, (runtimeVersion%100)/10);
	
	xd::Core::Log(LOG_DEVELOPER, L"%d CUDA capable GPU device(s) detected.\n", deviceCount);	

	for (int devid = 0; devid < deviceCount; devid++) {
		cudaDeviceProp *pdp = &deviceProp[devid];

		cudaGetDeviceProperties(pdp, devid);
		if (pdp->major < 1) {
			xd::Core::Error(ERR_FATAL, L"MyCuda::Init: GPU device does not support CUDA.\n");
			return false;
		}
		
		xd::Core::Log(LOG_DEVELOPER, L"Device [%d]: %hs\n", 0, pdp->name);
		xd::Core::Log(LOG_DEVELOPER, L"Compute Capability version: %d.%d\n", pdp->major, pdp->minor);

		xd::Core::Log(LOG_DEVELOPER, L"GPU Clock rate: %0.2f GHz\n", pdp->clockRate * 1e-6f);
		xd::Core::Log(LOG_DEVELOPER, L"Memory Clock rate: %.0f Mhz\n", pdp->memoryClockRate * 1e-3f);
		xd::Core::Log(LOG_DEVELOPER, L"Memory Bus Width: %d bits\n", pdp->memoryBusWidth);
		xd::Core::Log(LOG_DEVELOPER, L"L2 Cache: %d bytes\n", pdp->l2CacheSize);
		
		xd::Core::Log(LOG_DEVELOPER, L"GMem: %.0f Mbytes\n", (float)pdp->totalGlobalMem/1048576.0f);
		xd::Core::Log(LOG_DEVELOPER, L"MP: %d, SP/MP: %d\n", pdp->multiProcessorCount, ConvertSMVer2Cores(pdp->major, pdp->minor));
		xd::Core::Log(LOG_DEVELOPER, L"CMem: %u bytes\n", pdp->totalConstMem);
		xd::Core::Log(LOG_DEVELOPER, L"SMem/block: %u bytes\n", pdp->sharedMemPerBlock);
		xd::Core::Log(LOG_DEVELOPER, L"Reg/block: %d\n", pdp->regsPerBlock);
		xd::Core::Log(LOG_DEVELOPER, L"Warp size: %d\n", pdp->warpSize);
		xd::Core::Log(LOG_DEVELOPER, L"Maximum threads/MP: %d\n", pdp->maxThreadsPerMultiProcessor);	
		xd::Core::Log(LOG_DEVELOPER, L"Maximum threads/block %d\n", pdp->maxThreadsPerBlock);
		xd::Core::Log(LOG_DEVELOPER, L"Maximum block dimensions: %d x %d x %d\n", pdp->maxThreadsDim[0], pdp->maxThreadsDim[1], pdp->maxThreadsDim[2]);
		xd::Core::Log(LOG_DEVELOPER, L"Maximum grid dimensions: %d x %d x %d\n", pdp->maxGridSize[0], pdp->maxGridSize[1], pdp->maxGridSize[2]);
		xd::Core::Log(LOG_DEVELOPER, L"Maximum memory pitch: %u bytes\n", pdp->memPitch);
		xd::Core::Log(LOG_DEVELOPER, L"Texture alignment: %u bytes\n", pdp->textureAlignment);

		xd::Core::Log(LOG_DEVELOPER, L"Concurrent copy and execution: %s with %d copy engine(s)\n", (pdp->deviceOverlap ? L"Yes" : L"No"), pdp->asyncEngineCount);
		xd::Core::Log(LOG_DEVELOPER, L"Run time limit on kernels: %s\n", pdp->kernelExecTimeoutEnabled ? L"Yes" : L"No");
		xd::Core::Log(LOG_DEVELOPER, L"Integrated GPU sharing Host Memory: %s\n", pdp->integrated ? L"Yes" : L"No");
		xd::Core::Log(LOG_DEVELOPER, L"Support host page-locked memory mapping: %s\n", pdp->canMapHostMemory ? L"Yes" : L"No");
		xd::Core::Log(LOG_DEVELOPER, L"Concurrent kernel execution: %s\n", pdp->concurrentKernels ? L"Yes" : L"No");
        xd::Core::Log(LOG_DEVELOPER, L"Alignment requirement for Surfaces: %s\n", pdp->surfaceAlignment ? L"Yes" : L"No");
        xd::Core::Log(LOG_DEVELOPER, L"Device has ECC support enabled: %s\n", pdp->ECCEnabled ? L"Yes" : L"No");
        xd::Core::Log(LOG_DEVELOPER, L"Device is using TCC driver mode: %s\n", pdp->tccDriver ? L"Yes" : L"No");
        xd::Core::Log(LOG_DEVELOPER, L"Device supports Unified Addressing (UVA): %s\n", pdp->unifiedAddressing ? L"Yes" : L"No");
        xd::Core::Log(LOG_DEVELOPER, L"Device PCI Bus ID / PCI location ID: %d / %d\n", pdp->pciBusID, pdp->pciDeviceID);
	}

	cudaSetDevice(0);
		
	return true;
}

void MyCuda::Shutdown() {
	cudaDeviceReset();
}