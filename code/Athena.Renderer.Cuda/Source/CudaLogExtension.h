#ifndef __cuda_log_extension_h
#define __cuda_log_extension_h

#include <cuda.h>
#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <Log.h>


template <typename T>
void CUDACheck(T result, char const *const func, const char *const file, int const line)
{
	if (result)
	{
		LOG_TL(LogLevel::Error, "CUDA error at %s:%d code=%d(%s) \"%s\"", 
			file, 
			line, 
			static_cast<unsigned int>(result), 
			_cudaGetErrorEnum(result), 
			func);

		DEVICE_RESET
		// Make sure we call CUDA Device Reset before exiting
		exit(EXIT_FAILURE);
	}
}

#define CHECK_CUDA_ERROR_LOG_TL(val)		CUDACheck((val), #val, __FILE__, __LINE__)

#endif __cuda_log_extension_h
