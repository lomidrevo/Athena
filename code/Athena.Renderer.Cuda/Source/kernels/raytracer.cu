
#include <cuda.h>
#include <cuda_runtime.h>
#include <Camera.h>
#include <Convert.h>
#include <device_launch_parameters.h>
#include <Frame.h>
#include <HitResult.h>
#include <Objects.h>
#include <Ray.h>
#include <RayTracing.h>
#include <Rendering.h>
#include <TypeDefs.h>
#include <Vectors.h>
#include "../CudaLogExtension.h"
#include "../AthenaCuda.h"

#define BOX_COUNT_DEVICE_LIMIT			512
#define SPHERE_COUNT_DEVICE_LIMIT		512


// __constant__ memory
__device__ __constant__ double3 cameraParams[CameraParameter::Count];

// __constant__ memory accessors
__device__ const v3f& Camera(CameraParameter::Enum parameterId) { return (v3f&)cameraParams[parameterId]; }
__device__ const v3f* Camera() { return (v3f*)cameraParams; }


__device__ f32 GetRandom(ui32* seed0, ui32* seed1)
{
	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	ui32 ires = ((*seed0) << 16) + (*seed1);

	/* use union struct to convert int to float */
	union
	{
		f32 f;
		ui32 ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;  /* bitwise AND, bitwise OR */
	return (res.f - 2.0f) / 2.0f;
}

__global__ void RayTracingKernel
(
	const Objects objects,
	v4b* colorBuffer,
	v4b* depthBuffer,
	v4b* normalBuffer,
	v4b* debugBuffer,
	v1ui64* objectIdBuffer,

	const v2ui pixelSize,
	const ui64 frameCount
)
{
	// blockIdx: <0; gridDim)
	// threadIdx: <0; blockDim)

	// pixel position in output image
	// x: left -> right
	// y: top -> bottom

	const v2ui pixel(
		pixelSize.x * (blockIdx.x * blockDim.x + threadIdx.x),
		pixelSize.y * (blockIdx.y * blockDim.y + threadIdx.y));
	const ui32 index = (pixel.y * blockDim.x * pixelSize.x * gridDim.x + pixel.x);

	// thread colors
	v2f threadColor((real)threadIdx.x / blockDim.x, (real)threadIdx.y / blockDim.y);
	// block color
	const real blockColor =
		((blockIdx.x % 2 == 0 && blockIdx.y % 2 != 0) || (blockIdx.x % 2 != 0 && blockIdx.y % 2 == 0)) ?
		((real)blockIdx.x / gridDim.x) * ((real)blockIdx.y / gridDim.y) : 0;

	//RenderingParameters params;
	//params.tracingMethod = TracingMethod::StraightForward;

	Ray primaryRay = Ray::GetPrimary(Camera(), pixel, pixelSize);

	// TODO raytracing
	RayTraceResult result;// = RayTrace(
		//objects, 
		//params, 
		//primaryRay, 
		//array_of<v3f>(), 
		//null, 
		//null, 
		//0);

	ui32 seed0 = pixel.x * (ui32)frameCount;
	ui32 seed1 = pixel.y * (ui32)frameCount;
	result.color.Set(
		REAL_TO_BYTE(GetRandom(&seed0, &seed1)),
		REAL_TO_BYTE(GetRandom(&seed0, &seed1)),
		REAL_TO_BYTE(GetRandom(&seed0, &seed1)));

	// color is average since last view update
	//frame.colorAccBuffer[frameOffset] += result.color;
	//result.color = frame.colorAccBuffer[frameOffset] / (real)frameCountSinceChange;

	// color
	colorBuffer[index] = Vector3fToVector4b(result.color);
	
	// depth
	depthBuffer[index] = v4b();
	
	// normal
	normalBuffer[index] = Vector3fToVector4b(vectors::Abs(primaryRay.direction));

	// debug
	debugBuffer[index].x = REAL_TO_BYTE(threadColor.x);
	debugBuffer[index].y = REAL_TO_BYTE(threadColor.y);
	debugBuffer[index].z = REAL_TO_BYTE(blockColor);
}

// wrapper for the __global__ call that sets up the kernel call
EXTERN_C void RunRayTracingKernel(const Objects& objects, 
	dim3 numOfBlocks, dim3 threadsPerBlock, v2ui pixelSize, AthenaCudaStorage* storage, const ui64 frameCount)
{
	// execute the kernel
	RayTracingKernel <<< numOfBlocks, threadsPerBlock >>>
	(
		objects,
		storage->buffer[FrameBuffer::Color].ptr,
		storage->buffer[FrameBuffer::Depth].ptr,
		storage->buffer[FrameBuffer::Normal].ptr,
		storage->buffer[FrameBuffer::Debug].ptr,
		storage->objectIdBuffer.ptr,
		pixelSize,
		frameCount
	);

	CHECK_CUDA_ERROR_LOG_TL(cudaThreadSynchronize());
}

#define COPY_OBJECTS_TO_DEVICE(o) \
	CHECK_CUDA_ERROR_LOG_TL(cudaMemcpyToSymbol(o, objects->o.array.ptr, sizeof(*o) * objects->o.array.count));

EXTERN_C void UpdateKernel(const v3f* camera)
{
	// copy camera properties to device
	CHECK_CUDA_ERROR_LOG_TL(cudaMemcpyToSymbol(cameraParams, camera, sizeof(*cameraParams) * CameraParameter::Count));
}
