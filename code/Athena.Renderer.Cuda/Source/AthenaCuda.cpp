
#include "AthenaCuda.h"
#include "CudaLogExtension.h"
#include <cuda_runtime.h>
#include <Camera.h>
#include <device_launch_parameters.h>
#include <Frame.h>
#include <Log.h>
#include <Objects.h>
#include <StringHelpers.h>


EXTERN_C void UpdateKernel(const v3f* camera);
// extern function for launching cuda kernel
EXTERN_C void RunRayTracingKernel(const Objects& objects, dim3 numOfBlocks, dim3 threadsPerBlock, 
	v2ui pixelSize, AthenaCudaStorage* storage, const ui64 frameCount);


AthenaCudaStorage storage;


// shows information about available cuda devices
void QueryCudaDevices()
{
	// number of CUDA devices
	int devCount;
	CHECK_CUDA_ERROR_LOG_TL(cudaGetDeviceCount(&devCount));

	// iterate through devices
	for (int i = 0; i < devCount; ++i)
	{
		// get device properties
		cudaDeviceProp devProp;
		CHECK_CUDA_ERROR_LOG_TL(cudaGetDeviceProperties(&devProp, i));

		LOG_TL(LogLevel::Info, "[CUDA] %s:", devProp.name);
		LOG_TL(LogLevel::Info, "\tMajor revision number:         %d", devProp.major);
		LOG_TL(LogLevel::Info, "\tMinor revision number:         %d", devProp.minor);
		LOG_TL(LogLevel::Info, "\tTotal global memory:           %u", devProp.totalGlobalMem);
		LOG_TL(LogLevel::Info, "\tTotal shared memory per block: %u", devProp.sharedMemPerBlock);
		LOG_TL(LogLevel::Info, "\tTotal registers per block:     %d", devProp.regsPerBlock);
		LOG_TL(LogLevel::Info, "\tWarp size:                     %d", devProp.warpSize);
		LOG_TL(LogLevel::Info, "\tMaximum memory pitch:          %u", devProp.memPitch);
		LOG_TL(LogLevel::Info, "\tMaximum threads per block:     %d", devProp.maxThreadsPerBlock);
		LOG_TL(LogLevel::Info, "\tMaximum dimensions of block:   %dx%dx%d",
			devProp.maxThreadsDim[0], devProp.maxThreadsDim[1], devProp.maxThreadsDim[2]);
		LOG_TL(LogLevel::Info, "\tMaximum dimensions of grid:    %dx%dx%d",
			devProp.maxGridSize[0], devProp.maxGridSize[1], devProp.maxGridSize[2]);
		LOG_TL(LogLevel::Info, "\tClock rate:                    %d", devProp.clockRate);
		LOG_TL(LogLevel::Info, "\tTotal constant memory:         %u", devProp.totalConstMem);
		LOG_TL(LogLevel::Info, "\tTexture alignment:             %u", devProp.textureAlignment);
		LOG_TL(LogLevel::Info, "\tConcurrent copy and execution: %s", (devProp.deviceOverlap ? "Yes" : "No"));
		LOG_TL(LogLevel::Info, "\tNumber of multiprocessors:     %d", devProp.multiProcessorCount);
		LOG_TL(LogLevel::Info, "\tKernel execution timeout:      %s", (devProp.kernelExecTimeoutEnabled ? "Yes" : "No"));
	}
}

ATHENA_CUDA_DLL_EXPORT ATHENA_CUDA_INITIALIZE(Initialize)
{
	char tmpBuffer[256] = {};

	using namespace Common::Strings;

	QueryCudaDevices();

	// initialize device frame buffers
	const uint32 frameBufferLength = frameSize.x * frameSize.y * sizeof(v4b);
	for (int i = 0; i < FrameBuffer::Count; i++)
	{
		CHECK_CUDA_ERROR_LOG_TL(cudaMalloc((void**)&storage.buffer[i].ptr, frameBufferLength));
		storage.buffer[i].count = frameSize.x * frameSize.y;
		LOG_TL(LogLevel::Info, "__device__ FrameBuffer::%s created [%s]", 
			FrameBuffer::GetString((FrameBuffer::Enum)i), GetMemSizeString(tmpBuffer, frameBufferLength));
	}
	const uint32 objectIdBufferLength = frameSize.x * frameSize.y * sizeof(v1ui64);
	CHECK_CUDA_ERROR_LOG_TL(cudaMalloc((void**)&storage.objectIdBuffer.ptr, objectIdBufferLength));
	storage.objectIdBuffer.count = frameSize.x * frameSize.y;
	LOG_TL(LogLevel::Info, "__device__ objectId buffer created [%s]", GetMemSizeString(tmpBuffer, objectIdBufferLength));
}

template <typename T>
void CopyToDevice(list_of<T>& destination, const list_of<T>& source, uint count = 512)
{
	count = MAX2(count, source.currentCount);

	if (!destination.array.ptr)
		CHECK_CUDA_ERROR_LOG_TL(cudaMalloc((void**)&destination.array.ptr, sizeof(T) * count));

	else if (destination.array.count < source.currentCount)
	{
		CHECK_CUDA_ERROR_LOG_TL(cudaFree(destination.array.ptr));
		CHECK_CUDA_ERROR_LOG_TL(cudaMalloc((void**)&destination.array.ptr, sizeof(T) * count));
	}
	destination.array.count = count;

	CHECK_CUDA_ERROR_LOG_TL(cudaMemcpy(destination.array.ptr, (void*)source.array.ptr, sizeof(T) * source.currentCount,
		cudaMemcpyHostToDevice));
	destination.currentCount = source.currentCount;
}

ATHENA_CUDA_DLL_EXPORT ATHENA_CUDA_UPDATE(Update)
{
	// copy objects to device
	CopyToDevice<Box>(deviceObjects.boxes, objects.boxes);
	CopyToDevice<Sphere>(deviceObjects.spheres, objects.spheres);
	CopyToDevice<ObjectId>(deviceObjects.everything, objects.everything);

	UpdateKernel(camera);
}

// wrapper for the __global__ call that sets up the kernel call
ATHENA_CUDA_DLL_EXPORT ATHENA_CUDA_RENDER(Render)
{
	// execute the kernel
	RunRayTracingKernel(
		objects,
		dim3(regions.x, regions.y, 1), 
		dim3(regionSize.x, regionSize.y, 1), 
		pixelSize,
		&storage,
		frame->count);

	// TODO zbytocne kopirujem cele polia, ked je pixelSize > 1, staci renderovat do mensej casti a kopirovat len tu ?

	// copy host frame buffers to device
	for (int i = 0; i < FrameBuffer::Count; i++)
	{
		CHECK_CUDA_ERROR_LOG_TL(cudaMemcpy(frame->buffer[i].ptr, (void*)storage.buffer[i].ptr,
			frame->buffer[i].count * sizeof(v4b), cudaMemcpyDeviceToHost));
	}
	CHECK_CUDA_ERROR_LOG_TL(cudaMemcpy(frame->objectIdBuffer.ptr, (void*)storage.objectIdBuffer.ptr,
		frame->objectIdBuffer.count * sizeof(v1ui64), cudaMemcpyDeviceToHost));
}

template <typename T>
void FreeOnDevice(list_of<T>& list)
{
	if (list.array.ptr)
	{
		CHECK_CUDA_ERROR_LOG_TL(cudaFree(list.array.ptr));
		list.array.ptr = null;
		list.array.count = 0;
		list.currentCount = 0;
	}
}

ATHENA_CUDA_DLL_EXPORT ATHENA_CUDA_CLEAN_UP(CleanUp)
{
	// free device frame buffers
	for (int i = 0; i < FrameBuffer::Count; i++)
	{
		CHECK_CUDA_ERROR_LOG_TL(cudaFree(storage.buffer[i].ptr));
		storage.buffer[i].ptr = null;
		storage.buffer[i].count = 0;
		LOG_TL(LogLevel::Info, "__device__ FrameBuffer::%s destroyed", FrameBuffer::GetString((FrameBuffer::Enum)i));
	}
	CHECK_CUDA_ERROR_LOG_TL(cudaFree(storage.objectIdBuffer.ptr));
	storage.objectIdBuffer.ptr = null;
	storage.objectIdBuffer.count = 0;
	LOG_TL(LogLevel::Info, "__device__ objectId buffer destroyed");

	FreeOnDevice<Box>(objects.boxes);
	FreeOnDevice<Sphere>(objects.spheres);
	FreeOnDevice<ObjectId>(objects.everything);
}
