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
	Vector3* velocityIn, Vector3* velocityOut, int arrSize);
bool CUDA_init(int arrSize);
bool CUDA_free();

//__global__
//void CUDA_SphereSphereCheck(Vector3* cuda_pos, float* cuda_radius,
//	Vector3* cuda_globalOnA, Vector3* cuda_globalOnB,
//	Vector3* cuda_normal, float* cuda_penetration, int* cuda_nodeAIndex, int* cuda_nodeBIndex, int* cuda_arrSize)
//{
//	int start = blockIdx.x * blockDim.x + threadIdx.x;
//	int collPairIndex = 0;
//	for (int i = 1; i <= start; ++i)
//	{
//		collPairIndex += (*cuda_arrSize - i);
//	}
//	for (int j = start + 1; j < *cuda_arrSize; ++j)
//	{
//		Vector3 itoj = cuda_pos[start] - cuda_pos[j];
//		float length = itoj.Length();
//
//		if (length < (cuda_radius[start] + cuda_radius[j]))		// collision detected
//		{
//			// i and j form a collison pair, from which we must construct a collision manifold
//			cuda_globalOnA[collPairIndex] = cuda_pos[start] - (itoj.Normalise() * cuda_radius[start]);
//			cuda_globalOnB[collPairIndex] = cuda_pos[j] + (itoj.Normalise() * cuda_radius[j]);
//			cuda_normal[collPairIndex] = -itoj.Normalise();
//			cuda_penetration[collPairIndex] = length - (cuda_radius[start] + cuda_radius[j]);
//			cuda_nodeAIndex[collPairIndex] = start;
//			cuda_nodeBIndex[collPairIndex] = j;
//		}
//		else	// no collision detected
//		{
//			cuda_globalOnA[collPairIndex].ToZero();
//			cuda_globalOnB[collPairIndex].ToZero();
//			cuda_normal[collPairIndex].ToZero();
//			cuda_penetration[collPairIndex] = 0;
//			cuda_nodeAIndex[collPairIndex] = -1;
//			cuda_nodeBIndex[collPairIndex] = -1;
//		}
//		++collPairIndex;
//	}
//}

__global__
void CUDA_SphereSphereCheck(Vector3* cuda_pos, float* cuda_radius,
	Vector3* cuda_velocity, Vector3* cuda_velocityOut, int* cuda_arrSize)
{
	int start = blockIdx.x * blockDim.x + threadIdx.x;
	int collPairIndex = 0;
	for (int i = 1; i <= start; ++i)
	{
		collPairIndex += (*cuda_arrSize - i);
	}
	for (int j = start + 1; j < *cuda_arrSize; ++j)
	{
		Vector3 itoj = cuda_pos[j] - cuda_pos[start];
		float length = itoj.Length();

		if (length < (cuda_radius[start] + cuda_radius[j]))		// collision detected
		{
			// start and j form a collison pair
			Vector3 itojN = itoj / length;

			float itojNVel = Vector3::Dot(cuda_velocity[j] - cuda_velocity[start], itojN);
			float jn = -(itojNVel * (1.5f));

			float overlap = (cuda_radius[start] + cuda_radius[j]) - length;
			float b = overlap * 0.05f * 60.0f;

			jn += b;

			jn = jn > 0.0f ? jn : 0.0f;

			cuda_velocityOut[start] -= itojN * (jn * 0.5f);
			cuda_velocityOut[j] += itojN * (jn * 0.5f);
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
Vector3* cuda_velocity = 0;
Vector3* cuda_velocityOut = 0;
int* cuda_arrSize = 0;

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
	cudaStatus = cudaMalloc((void**)&cuda_velocity, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		success = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_velocityOut, maxCollPairs * sizeof(Vector3));
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
	cudaFree(cuda_velocity);
	cudaFree(cuda_velocityOut);
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
	Vector3* velocityIn, Vector3* velocityOut, int arrSize)
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
	cudaStatus = cudaMemcpy(cuda_velocity, velocityIn, arrSize * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	int maxCollPairs = arrSize * arrSize * 0.5;
	cudaStatus = cudaMemcpy(cuda_velocityOut, velocityIn, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}

	int blockSize = 256;
	// calculate gridSize from https://devblogs.nvidia.com/parallelforall/even-easier-introduction-cuda/
	int gridSize = (arrSize + blockSize - 1) / blockSize;
	if (!error)
		CUDA_SphereSphereCheck << <gridSize, 600 >> >(cuda_pos, cuda_radius, cuda_velocity, cuda_velocityOut, cuda_arrSize);

	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);

	// copy outputs back to host memory
	cudaStatus = cudaMemcpy(velocityOut, cuda_velocityOut, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	//cudaStatus = cudaMemcpy(normal, cuda_normal, maxCollPairs * sizeof(Vector3), cudaMemcpyDeviceToHost);
	//if (cudaStatus != cudaSuccess)
	//	fprintf(stderr, "cudaMalloc failed!");
	//cudaStatus = cudaMemcpy(penetration, cuda_penetration, maxCollPairs * sizeof(float), cudaMemcpyDeviceToHost);
	//if (cudaStatus != cudaSuccess)
	//	fprintf(stderr, "cudaMalloc failed!");
	//cudaStatus = cudaMemcpy(indexA, cuda_nodeAIndex, maxCollPairs * sizeof(int), cudaMemcpyDeviceToHost);
	//if (cudaStatus != cudaSuccess)
	//	fprintf(stderr, "cudaMalloc failed!");
	//cudaStatus = cudaMemcpy(indexB, cuda_nodeBIndex, maxCollPairs * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	return;
}
