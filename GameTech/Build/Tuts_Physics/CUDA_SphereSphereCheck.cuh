#pragma once

#include "../nclgl/Vector3.h"

extern "C" void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int arrSize);

extern "C" bool CUDA_init(int arrSize);
extern "C" bool CUDA_free();