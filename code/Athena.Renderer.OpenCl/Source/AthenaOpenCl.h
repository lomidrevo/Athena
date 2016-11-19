#ifndef __athena_opencl_h
#define __athena_opencl_h

#include <Athena.h>
#include <Array.h>
#include <CL\cl.hpp>
#include <Frame.h>
#include <TypeDefs.h>

#define ATHENA_OPENCL_DLL_EXPORT		EXTERN_C DLL_EXPORT
#define ATHENA_OPENCL_INITIALIZE		ATHENA_GPU_INITIALIZE
#define ATHENA_OPENCL_UPDATE			ATHENA_GPU_UPDATE
#define ATHENA_OPENCL_RENDER			ATHENA_GPU_RENDER
#define ATHENA_OPENCL_CLEAN_UP			ATHENA_GPU_CLEAN_UP

struct AthenaOpenClStorage
{
	cl::Platform platform;
	cl::Device device;
	cl::Context context;
	cl::Program program;
	cl::Kernel kernel;
	cl::CommandQueue queue;

	cl::Buffer buffer[FrameBuffer::Count];
	v2ui frameSize;
};

#endif __athena_opencl_h
