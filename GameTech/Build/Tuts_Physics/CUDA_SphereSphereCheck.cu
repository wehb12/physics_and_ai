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

#pragma once

// CUDA includes
#include <cuda_runtime.h>
#include <vector_types.h>
#include <iostream>

#include "CUDA_SphereSphereCheck.cuh"

void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int arrSize);

__global__
void CUDA_SphereSphereCheck(Vector3* cuda_pos, float* cuda_radius,
	Vector3* cuda_globalOnA, Vector3* cuda_globalOnB,
	Vector3* cuda_normal, float* cuda_penetration, int* cuda_arrSize)
{
	for (int i = 0; i < *cuda_arrSize - 1; ++i)
	{
		for (int j = i + 1; j < *cuda_arrSize; ++j)
		{
			Vector3 itoj = cuda_pos[i] - cuda_pos[j];
			float length = itoj.Length();

			if (length < (cuda_radius[i] + cuda_radius[j]))
			{

			}
		}
	}
}

void CUDA_run(Vector3* positions, float* radii,
	Vector3* globalOnA, Vector3* globalOnB,
	Vector3* normal, float* penetration, int arrSize)
{
	Vector3* cuda_pos = 0;
	float* cuda_radius = 0;
	Vector3* cuda_globalOnA = 0;
	Vector3* cuda_globalOnB = 0;
	Vector3* cuda_normal = 0;
	float* cuda_penetration = 0;
	int* cuda_arrSize = 0;
	cudaError_t cudaStatus;
	bool error = false;

	// Error checking code from the default CUDA VS project
	//
	// Choose which GPU to run on, change this on a multi-GPU system.
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		error = true;
	}

	// Allocate GPU buffers for all data
	cudaStatus = cudaMalloc((void**)&cuda_pos, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_radius, arrSize * sizeof(float));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_globalOnA, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_globalOnB, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_normal, arrSize * sizeof(Vector3));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_penetration, arrSize * sizeof(float));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}
	cudaStatus = cudaMalloc((void**)&cuda_arrSize, sizeof(int));
	if (cudaStatus != cudaSuccess)
	{
		fprintf(stderr, "cudaMalloc failed!");
		error = true;
	}

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

	if (!error)
		CUDA_SphereSphereCheck<<<1, 1>>>(cuda_pos, cuda_radius, cuda_globalOnA, cuda_globalOnB, cuda_normal, cuda_penetration, cuda_arrSize);

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
	cudaStatus = cudaMemcpy(globalOnA, cuda_globalOnA, arrSize * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(globalOnB, cuda_globalOnB, arrSize * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(normal, cuda_normal, arrSize * sizeof(Vector3), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	cudaStatus = cudaMemcpy(penetration, cuda_penetration, arrSize * sizeof(float), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	cudaFree(cuda_pos);
	cudaFree(cuda_radius);
	cudaFree(cuda_globalOnA);
	cudaFree(cuda_globalOnB);
	cudaFree(cuda_normal);
	cudaFree(cuda_penetration);
	cudaFree(cuda_arrSize);

	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaDeviceReset failed!");

	return;
}


//#include "cuda_runtime.h"
//#include "device_launch_parameters.h"
//#include "CUDA_SphereSphereCheck.cuh"
//
//#include <stdio.h>
//
//cudaError_t addWithCuda(int *c, const int *a, const int *b, unsigned int size);
//
//__global__ void addKernel(int *c, const int *a, const int *b)
//{
//	int i = threadIdx.x;
//	c[i] = a[i] + b[i];
//}
//
//int CUDA_run()
//{
//	const int arraySize = 5;
//	const int a[arraySize] = { 1, 2, 3, 4, 5 };
//	const int b[arraySize] = { 10, 20, 30, 40, 50 };
//	int c[arraySize] = { 0 };
//
//	// Add vectors in parallel.
//	cudaError_t cudaStatus = addWithCuda(c, a, b, arraySize);
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "addWithCuda failed!");
//		return 1;
//	}
//
//	printf("{1,2,3,4,5} + {10,20,30,40,50} = {%d,%d,%d,%d,%d}\n",
//		c[0], c[1], c[2], c[3], c[4]);
//
//	// cudaDeviceReset must be called before exiting in order for profiling and
//	// tracing tools such as Nsight and Visual Profiler to show complete traces.
//	cudaStatus = cudaDeviceReset();
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaDeviceReset failed!");
//		return 1;
//	}
//
//	return 0;
//}
//
//// Helper function for using CUDA to add vectors in parallel.
//cudaError_t addWithCuda(int *c, const int *a, const int *b, unsigned int size)
//{
//	int *dev_a = 0;
//	int *dev_b = 0;
//	int *dev_c = 0;
//	cudaError_t cudaStatus;
//
//	// Choose which GPU to run on, change this on a multi-GPU system.
//	cudaStatus = cudaSetDevice(0);
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
//		goto Error;
//	}
//
//	// Allocate GPU buffers for three vectors (two input, one output)    .
//	cudaStatus = cudaMalloc((void**)&dev_c, size * sizeof(int));
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMalloc failed!");
//		goto Error;
//	}
//
//	cudaStatus = cudaMalloc((void**)&dev_a, size * sizeof(int));
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMalloc failed!");
//		goto Error;
//	}
//
//	cudaStatus = cudaMalloc((void**)&dev_b, size * sizeof(int));
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMalloc failed!");
//		goto Error;
//	}
//
//	// Copy input vectors from host memory to GPU buffers.
//	cudaStatus = cudaMemcpy(dev_a, a, size * sizeof(int), cudaMemcpyHostToDevice);
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMemcpy failed!");
//		goto Error;
//	}
//
//	cudaStatus = cudaMemcpy(dev_b, b, size * sizeof(int), cudaMemcpyHostToDevice);
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMemcpy failed!");
//		goto Error;
//	}
//
//	// Launch a kernel on the GPU with one thread for each element.
//	addKernel <<<1, size >>>(dev_c, dev_a, dev_b);
//
//	// Check for any errors launching the kernel
//	cudaStatus = cudaGetLastError();
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
//		goto Error;
//	}
//
//	// cudaDeviceSynchronize waits for the kernel to finish, and returns
//	// any errors encountered during the launch.
//	cudaStatus = cudaDeviceSynchronize();
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
//		goto Error;
//	}
//
//	// Copy output vector from GPU buffer to host memory.
//	cudaStatus = cudaMemcpy(c, dev_c, size * sizeof(int), cudaMemcpyDeviceToHost);
//	if (cudaStatus != cudaSuccess) {
//		fprintf(stderr, "cudaMemcpy failed!");
//		goto Error;
//	}
//
//Error:
//	cudaFree(dev_c);
//	cudaFree(dev_a);
//	cudaFree(dev_b);
//
//	return cudaStatus;
//}
