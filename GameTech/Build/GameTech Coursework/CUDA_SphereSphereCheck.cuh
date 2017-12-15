#pragma once

#include "../nclgl/Vector3.h"

extern "C" void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int* cuda_nodeAIndex,
	int* cuda_nodeBIndex, int arrSize);

extern "C" bool CUDA_init(int arrSize);
extern "C" bool CUDA_free();

//#pragma once
//
//#include "../nclgl/Vector3.h"
//
//extern "C" void CUDA_run(Vector3* positions, float* radii,
//	Vector3* velocityIn, Vector3* veolcityOut, int arrSize);
//
//extern "C" bool CUDA_init(int arrSize);
//extern "C" bool CUDA_free();