
static float GetRandom(uint *seed0, uint *seed1)
{
	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	uint ires = ((*seed0) << 16) + (*seed1);

	/* use union struct to convert int to float */
	union
	{
		float f;
		uint ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;  /* bitwise AND, bitwise OR */
	return (res.f - 2.0f) / 2.0f;
}

__kernel void RayTracingKernel
(
	const uint2 frameSize, 

	global uchar4* colorBuffer,
	global uchar4* depthBuffer,
	global uchar4* normalBuffer,
	global uchar4* debugBuffer
)
{
	const int index = get_global_id(0);
	//const int globalSize = get_global_size(0);
	//const int localIndex = get_local_id(0);
	//const int localSize = get_local_size(0);

	//const uint regionSize_x = 16;
	//const uint regionSize_y = 16;
	//const uint region_x = globalId % 80;
	//const uint region_y = globalId / 80;
	//const uint regionIndex = region_y * (1280 * regionSize_y) + region_x * regionSize_x;
	//const uint localSize_x = 1;
	//const uint local_x = localIndex % localSize_x;
	//const uint local_y = localIndex / localSize_x;

	const uint2 pixel;
	pixel.x = index % frameSize.x;
	pixel.y = index / frameSize.x;

	uint seed0 = pixel.x;
	uint seed1 = pixel.y;

	// color
	colorBuffer[index].x = GetRandom(&seed0, &seed1) * 255;
	colorBuffer[index].y = GetRandom(&seed0, &seed1) * 255;
	colorBuffer[index].z = GetRandom(&seed0, &seed1) * 255;
	colorBuffer[index].w = 0;

	// depth
	depthBuffer[index].x = 0;
	depthBuffer[index].y = 0;
	depthBuffer[index].z = 0;
	depthBuffer[index].w = 0;

	// normal
	normalBuffer[index].x = 0;
	normalBuffer[index].y = 0;
	normalBuffer[index].z = 0;
	normalBuffer[index].w = 0;

	// debug
	debugBuffer[index].x = (uchar)((float)pixel.x / frameSize.x * 255);
	debugBuffer[index].y = (uchar)((float)pixel.y / frameSize.y * 255);
	debugBuffer[index].z = 0;
	debugBuffer[index].w = 0;
}
