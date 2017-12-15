/******************************************************************************
Code: CUDA implementation of a sphere-sphere collision check
Implements:
Author:
Will Hinds      <w.hinds2@newcastle.ac.uk>
Description:

Carries out a sphere-sphere collision check on several spherical entities
then populates the data structures with the necessary info to compute a
collision manifold (of one point for a sphere-sphere collision)

*//////////////////////////////////////////////////////////////////////////////


// CUDA includes
#include <cuda_runtime.h>
#include <vector_types.h>
#include <iostream>

#include "../nclgl/Vector3.h"

#include "CUDA_SphereSphereCheck.cuh"

void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int* cuda_nodeAIndex,
	int* cuda_nodeBIndex, int arrSize);
bool CUDA_init(int arrSize);
bool CUDA_free();

__global__
void CUDA_SphereSphereCheck(Vector3* cuda_pos, float* cuda_radius,
	Vector3* cuda_globalOnA, Vector3* cuda_globalOnB,
	Vector3* cuda_normal, float* cuda_penetration, int* cuda_nodeAIndex, int* cuda_nodeBIndex, int* cuda_arrSize)
{
	int start = blockIdx.x * blockDim.x + threadIdx.x;
	int collPairIndex = 0;
	for (int i = 1; i <= start; ++i)
	{
		collPairIndex += (*cuda_arrSize - i);
	}
	for (int j = start + 1; j < *cuda_arrSize; ++j)
	{
		Vector3 itoj = cuda_pos[start] - cuda_pos[j];
		float length = itoj.Length();

		if (length < (cuda_radius[start] + cuda_radius[j]))		// collision detected
		{
			// i and j form a collison pair, from which we must construct a collision manifold
			cuda_globalOnA[collPairIndex] = cuda_pos[start] - (itoj.Normalise() * cuda_radius[start]);
			cuda_globalOnB[collPairIndex] = cuda_pos[j] + (itoj.Normalise() * cuda_radius[j]);
			cuda_normal[collPairIndex] = -itoj.Normalise();
			cuda_penetration[collPairIndex] = length - (cuda_radius[start] + cuda_radius[j]);
			cuda_nodeAIndex[collPairIndex] = start;
			cuda_nodeBIndex[collPairIndex] = j;
		}
		else	// no collision detected
		{
			cuda_globalOnA[collPairIndex].ToZero();
			cuda_globalOnB[collPairIndex].ToZero();
			cuda_normal[collPairIndex].ToZero();
			cuda_penetration[collPairIndex] = 0;
			cuda_nodeAIndex[collPairIndex] = -1;
			cuda_nodeBIndex[collPairIndex] = -1;
		}
		++collPairIndex;
	}
}

Vector3* cuda_pos = 0;
float* cuda_radius = 0;
Vector3* cuda_globalOnA = 0;
Vector3* cuda_globalOnB = 0;
Vector3* cuda_normal = 0;
float* cuda_penetration = 0;
int* cuda_arrSize = 0;
int* cuda_nodeAIndex = 0;
int* cuda_nodeBIndex = 0;

bool CUDA_init(int arrSize)
{
	cudaError_t cudaStatus;
	bool success = true;
	// Error checking code from the default CUDA VS project
	//
	// Choose which GPU to run on, change this on a multi-GPU system.
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		success = true;
	}

	// Allocate GPU buffers for all data
	cudaStatus = cudaMalloc((void**)&cuda_pos, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_radius, arrSize * sizeof(float));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}

	int maxCollPairs = arrSize * arrSize * 0.5;
	cudaStatus = cudaMalloc((void**)&cuda_globalOnA, maxCollPairs * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_globalOnB, maxCollPairs * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_normal, maxCollPairs * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_penetration, maxCollPairs * sizeof(float));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_nodeAIndex, maxCollPairs * sizeof(int));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_nodeBIndex, maxCollPairs * sizeof(int));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_arrSize, sizeof(int));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}

	return success;
}

bool CUDA_free()
{
	cudaError_t cudaStatus;

	cudaFree(cuda_pos);
	cudaFree(cuda_radius);
	cudaFree(cuda_globalOnA);
	cudaFree(cuda_globalOnB);
	cudaFree(cuda_normal);
	cudaFree(cuda_penetration);
	cudaFree(cuda_nodeAIndex);
	cudaFree(cuda_nodeBIndex);
	cudaFree(cuda_arrSize);

	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaDeviceReset failed!");
		return false;
	}

	return true;
}

void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int* indexA,
	int* indexB, int arrSize)
{
	cudaError_t cudaStatus;
	bool error = false;

	// copy data from host to GPU
	cudaStatus = cudaMemcpy(cuda_pos, positions, arrSize * sizeof(Vector3), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMemcpy(cuda_radius, radii, arrSize * sizeof(float), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMemcpy(cuda_arrSize, &arrSize, sizeof(int), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}

	int blockSize = 256;
	// calculate gridSize from https://devblogs.nvidia.com/parallelforall/even-easier-introduction-cuda/
	int gridSize = (arrSize + blockSize - 1) / blockSize;
	if (!error)
		CUDA_SphereSphereCheck << <gridSize, 600 >> >(cuda_pos, cuda_radius, cuda_globalOnA, cuda_globalOnB, cuda_normal, cuda_penetration, cuda_nodeAIndex, cuda_nodeBIndex, cuda_arrSize);

	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);

	int maxCollPairs = arrSize * arrSize * 0.5;
	// copy outputs back to host memory
	cudaStatus = cudaMemcpy(globalOnA, cuda_globalOnA, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(globalOnB, cuda_globalOnB, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(normal, cuda_normal, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(penetration, cuda_penetration, maxCollPairs * sizeof(float), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(indexA, cuda_nodeAIndex, maxCollPairs * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(indexB, cuda_nodeBIndex, maxCollPairs * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	return;
}
