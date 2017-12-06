
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>

extern "C" int CUDA_run();

void main()
{
	CUDA_run();
}