#ifndef __athena_cuda_h
#define __athena_cuda_h

#include <Athena.h>
#include <Array.h>
#include <Frame.h>
#include <TypeDefs.h>

#define ATHENA_CUDA_DLL_EXPORT		EXTERN_C DLL_EXPORT
#define ATHENA_CUDA_INITIALIZE		ATHENA_GPU_INITIALIZE
#define ATHENA_CUDA_UPDATE			ATHENA_GPU_UPDATE
#define ATHENA_CUDA_RENDER			ATHENA_GPU_RENDER
#define ATHENA_CUDA_CLEAN_UP		ATHENA_GPU_CLEAN_UP


struct AthenaCudaStorage
{
	array_of<v4b> buffer[FrameBuffer::Count];
	array_of<v1ui64> objectIdBuffer;
};

#endif __athena_cuda_h
