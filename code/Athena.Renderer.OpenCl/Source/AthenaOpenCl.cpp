
#include "AthenaOpenCl.h"
#include <Camera.h>
#include <CL\cl.hpp>
#include <Frame.h>
#include <Log.h>
#include <Objects.h>
#include "OpenClLogExtension.h"
#include <StringHelpers.h>


AthenaOpenClStorage storage;

// TODO OpenCL: dummy variables are required for memory alignment float3 is considered as float4 by OpenCL

// shows information about available opencl devices
void QueryDevices()
{
	std::vector<cl::Platform> platforms;
	CHECK_OPENCL_ERROR_LOG_TL(cl::Platform::get(&platforms));

	// TODO OpenCL: preco sa tu zobrazuju platforms/devices ktore nemaju poriadny popis ?!

	for (int i = 0; i < platforms.size(); i++)
	{
		std::vector<cl::Device> devices;
		CHECK_OPENCL_ERROR_LOG_TL(platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices));

		for (int i = 0; i < devices.size(); i++)
		{
			LOG_TL(LogLevel::Info, "[OpenCL] %s\\%s:",
				platforms[i].getInfo<CL_PLATFORM_NAME>(), devices[i].getInfo<CL_DEVICE_NAME>());
			LOG_TL(LogLevel::Info, "\tCL_DEVICE_VERSION:               %s", devices[i].getInfo<CL_DEVICE_VERSION>());
			LOG_TL(LogLevel::Info, "\tCL_DEVICE_MAX_WORK_GROUP_SIZE:   %u", 
				devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
			LOG_TL(LogLevel::Info, "\tCL_DEVICE_MAX_COMPUTE_UNITS:     %u", 
				devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
		}
	}
}

String LoadKernel(const char* kernelFile)
{
	FILE* fp = fopen(kernelFile, "rb");
	if (!fp)
	{
		LOG_TL(LogLevel::Error, "Failed to load kernel (%s)", kernelFile);
		exit(1);
	}

	String kernelSource;

	fseek(fp, 0L, SEEK_END);
	kernelSource.count = ftell(fp);
	rewind(fp);

	// TODO OpenCL: prerobit na MEM_ALLOC
	kernelSource.ptr = (char*)malloc(kernelSource.count + 1);
	kernelSource.ptr[kernelSource.count] = 0;
	fread(kernelSource.ptr, 1, kernelSource.count, fp);
	kernelSource.count++;

	fclose(fp);

	return kernelSource;

	LOG_TL(LogLevel::Info, "Kernel loaded (%s)", kernelFile);
}

ATHENA_OPENCL_DLL_EXPORT ATHENA_OPENCL_INITIALIZE(Initialize)
{
	using namespace Common::Strings;

	cl_int result;

	QueryDevices();

	std::vector<cl::Platform> platforms;
	CHECK_OPENCL_ERROR_LOG_TL(cl::Platform::get(&platforms));
	storage.platform = platforms[0];

	std::vector<cl::Device> devices;
	CHECK_OPENCL_ERROR_LOG_TL(storage.platform.getDevices(CL_DEVICE_TYPE_ALL, &devices));
	storage.device = devices[0];

	cl_context_properties contextProperties[3] = 
	{ 
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)(storage.platform()),
		0
	};
	storage.context = cl::Context(CL_DEVICE_TYPE_ALL, contextProperties, null, null, &result);
	CHECK_OPENCL_ERROR_LOG_TL(result);

	String kernelSource = LoadKernel("..\\Athena.Renderer.OpenCl\\Source\\kernels\\raytracer.cl");

	storage.program = cl::Program(storage.context, kernelSource.ptr, false, &result);
	CHECK_OPENCL_ERROR_LOG_TL(result);

	// TODO OpenCL: release kernelSource

	if (storage.program.build({ storage.device }) == CL_BUILD_PROGRAM_FAILURE)
	{
		std::string buildlog = storage.program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(storage.device);
		// TODO OpenCL: zalogovat buildLog

		exit(EXIT_FAILURE);
	}

	storage.kernel = cl::Kernel(storage.program, "RayTracingKernel", &result);
	CHECK_OPENCL_ERROR_LOG_TL(result);

	storage.queue = cl::CommandQueue(storage.context, storage.device, 0, &result);
	CHECK_OPENCL_ERROR_LOG_TL(result);

	char tmpBuffer[256] = {};

	// initialize device frame buffers
	const uint32 frameBufferLength = frameSize.x * frameSize.y * sizeof(cl_uchar4);
	for (int i = 0; i < FrameBuffer::Count; ++i)
	{
		storage.buffer[i] = cl::Buffer(storage.context, CL_MEM_WRITE_ONLY, frameBufferLength, &result);
		CHECK_OPENCL_ERROR_LOG_TL(result);

		LOG_TL(LogLevel::Info, "__opencl__ FrameBuffer::%s created [%s]",
			FrameBuffer::GetString((FrameBuffer::Enum)i), GetMemSizeString(tmpBuffer, frameBufferLength));
	}
	storage.frameSize = frameSize;

	// TODO OpenCL: create object buffer on device
	//openClBuffer = cl::Buffer(storage.context, CL_MEM_READ_ONLY, count * sizeof(something));
}

template <typename T>
void CopyToDevice(list_of<T>& destination, const list_of<T>& source, uint count = 512)
{
	// TODO OpenCL: copy objects to device
	//storage.queue.enqueueWriteBuffer(openClBuffer, CL_TRUE, 0, count * sizeof(something), cpuBuffer);
}

ATHENA_OPENCL_DLL_EXPORT ATHENA_OPENCL_UPDATE(Update)
{
	// set kernel parameters 
	storage.kernel.setArg(0, storage.frameSize);

	for (int i = 0; i < FrameBuffer::Count; ++i)
		storage.kernel.setArg(i + 1, storage.buffer[i]);
}

ATHENA_OPENCL_DLL_EXPORT ATHENA_OPENCL_RENDER(Render)
{
	std::size_t framePixelCount = storage.frameSize.x * storage.frameSize.y;
	std::size_t localWorkSize = storage.kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(storage.device);

	// Ensure the global work size is a multiple of local work size
	ASSERT(frameBufferPixelCount % localWorkSize == 0);

	CHECK_OPENCL_ERROR_LOG_TL(storage.queue.enqueueNDRangeKernel(storage.kernel, NULL, framePixelCount, localWorkSize));
	CHECK_OPENCL_ERROR_LOG_TL(storage.queue.finish());

	// read output from device
	const ui32 frameBufferSize = storage.frameSize.x * storage.frameSize.y * sizeof(cl_uchar4);
	for (int i = 0; i < FrameBuffer::Count; ++i)
	{
		CHECK_OPENCL_ERROR_LOG_TL(storage.queue.enqueueReadBuffer(storage.buffer[i], CL_TRUE, 0, frameBufferSize,
			frame->buffer[i].ptr));
	}
}

template <typename T>
void FreeOnDevice(list_of<T>& list)
{

}

ATHENA_OPENCL_DLL_EXPORT ATHENA_OPENCL_CLEAN_UP(CleanUp)
{

}
