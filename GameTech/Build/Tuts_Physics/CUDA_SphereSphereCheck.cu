#pragma once

// CUDA includes
#include<cuda_runtime.h>
#include<vector_types.h>

#include "../nclgl/Vector3.h"

__global__
void CUDA_SphereSphereCheck(Vector3* cu_pos, float* cu_radius,
	Vector3* cu_globalOnA, Vector3* cu_globalOnB,
	Vector3* cu_normal, float* cu_penetration, int entities)
{
	for (int i = 0; i < entities - 1; ++i)
	{
		for (int j = i + 1; j < entities; ++j)
		{
			Vector3 itoj = cu_pos[i] - cu_pos[j];
			float length = itoj.Length();

			if (length < (cu_radius[i] + cu_radius[j]))
			{

			}
		}
	}
}

void CUDA_run(Vector3* cu_pos, float* cu_radius,
	Vector3* cu_globalOnA, Vector3* cu_globalOnB,
	Vector3* cu_normal, float* cu_penetration, int entities)
{
	CUDA_SphereSphereCheck<<<1, 1>>>(positions, radii, globalOnA, globalOnB, normal, penetration, arrSize);
}
