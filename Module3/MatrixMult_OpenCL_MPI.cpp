#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <mpi.h>
#include <CL/cl.h>
#include <vector>


// =================================================================================================

// Size of the matrices - K, M, N (squared)
#define SIZE 15

// Threadblock sizes (e.g. for kernels myGEMM1 or myGEMM2)
#define TS 5

#define numSlaves 5

// =================================================================================================

// Set the kernel as a string (better to do this in a separate file though)
const char *kernelstring =
"__kernel void matrixMultiplication(const int M, const int N, const int K,"
"                      const __global int* A,"
"                      const __global int* B,"
"                      __global int* C) {"
"    const int globalRow = get_global_id(0);"
"    const int globalCol = get_global_id(1);"
"    float acc = 0.0f;"
"    for (int k=0; k<K; k++) {"
"        acc += A[k*M + globalRow] * B[globalCol*K + k];"
"    }"
"    C[globalCol*M + globalRow] = acc;"
"}";

// =================================================================================================

// Set the sizes
const int K = SIZE;
const int M = SIZE;
const int N = SIZE;

// Create the matrices
int matrixA[M][K];
int matrixB[K][N];
int matrixC[M][N];
int matrixAPortion[(SIZE / numSlaves) + (SIZE % numSlaves)][SIZE];
int matrixCPortion[(SIZE / numSlaves) + (SIZE % numSlaves)][SIZE];

MPI_Status mpiStatus;

void initialiseMatrices();
void printMatrixC();

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int numTasks;
	MPI_Comm_size(MPI_COMM_WORLD, &numTasks);

	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> end;

	// Configure the OpenCL environment
	cl_platform_id platform = 0;
	clGetPlatformIDs(1, &platform, NULL);
	cl_device_id device = 0;
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
	cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
	char deviceName[1024];
	clGetDeviceInfo(device, CL_DEVICE_NAME, 1024, deviceName, NULL);
	cl_event event = NULL;

	// Compile the kernel
	cl_program program = clCreateProgramWithSource(context, 1, &kernelstring, NULL, NULL);
	clBuildProgram(program, 0, NULL, "", NULL, NULL);

	int startingRow = 0;
	int numRowsMatrixA;
	if (rank == 0)
	{
		initialiseMatrices();

		start = std::chrono::steady_clock::now();
		// Begin clock and start matrix multiplication
		for (int destID = 1; destID <= numSlaves; destID++)
		{
			// Send starting point in matrixA
			MPI_Send(&startingRow, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

			// Send all of matrixB to slave.
			MPI_Send(&matrixB, SIZE*SIZE, MPI_INT, destID, 0, MPI_COMM_WORLD);

			// First slave gets any extra rows if dividing the rows does not equal a whole number.
			if (destID == 1)
			{
				numRowsMatrixA = (SIZE / numSlaves) + (SIZE % numSlaves);

				// Send number of rows
				MPI_Send(&numRowsMatrixA, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

				// Fill the portion with the segment of matrixA
				for (int i = 0; i < numRowsMatrixA; i++)
					for (int j = 0; j < SIZE; j++)
						matrixAPortion[i][j] = matrixA[startingRow + i][j];

				// Send chunk of matrixA
				MPI_Send(&matrixAPortion, numRowsMatrixA*SIZE, MPI_INT, destID, 0, MPI_COMM_WORLD);

				startingRow += numRowsMatrixA;
			}
			else
			{
				numRowsMatrixA = SIZE / numSlaves;

				// Send number of rows
				MPI_Send(&numRowsMatrixA, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

				// Fill the portion with the segment of matrixA
				for (int i = 0; i < numRowsMatrixA; i++)
					for (int j = 0; j < SIZE; j++)
						matrixAPortion[i][j] = matrixA[startingRow + i][j];

				// Send chunk of matrixA
				MPI_Send(&matrixAPortion, numRowsMatrixA*SIZE, MPI_INT, destID, 0, MPI_COMM_WORLD);

				startingRow += numRowsMatrixA;
			}
		}
		
		// wait for results from slaves
		for (int i = 1; i <= numSlaves; i++)
		{
			MPI_Recv(&startingRow, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);
			MPI_Recv(&numRowsMatrixA, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);
			MPI_Recv(&matrixCPortion, numRowsMatrixA*SIZE, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);

			int counter = 0;
			for (int j = startingRow; j < (startingRow + numRowsMatrixA); j++)
			{
				for (int k = 0; k < SIZE; k++)
					matrixC[j][k] = matrixCPortion[counter][k];
				counter++;
			}
		}
		end = std::chrono::steady_clock::now();

		//Clean-up OpenCL 
		clReleaseCommandQueue(queue);
		clReleaseProgram(program);

		printMatrixC();
		std::cout << "\nElapsed time in nanoseconds: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns\n\n";
	}

	//////////////////////////////////////////////////
	// Slave Receiving
	//////////////////////////////////////////////////
	if (rank > 0)
	{
		// Receive starting row for this slave
		MPI_Recv(&startingRow, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive full matrixB
		MPI_Recv(&matrixB, SIZE*SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive row count
		MPI_Recv(&numRowsMatrixA, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive portion of matrixA
		MPI_Recv(&matrixAPortion, numRowsMatrixA*SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);

		// Prepare OpenCL memory objects
		
		cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, numRowsMatrixA * SIZE * sizeof(int), NULL, NULL);
		cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, K*N * sizeof(int), NULL, NULL);
		cl_mem bufC = clCreateBuffer(context, CL_MEM_READ_WRITE, numRowsMatrixA * SIZE * sizeof(int), NULL, NULL);

		// Copy matrices to the GPU
	
		clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, numRowsMatrixA * SIZE * sizeof(int), matrixAPortion, 0, NULL, NULL);
		clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, SIZE * SIZE * sizeof(int), matrixB, 0, NULL, NULL);
		clEnqueueWriteBuffer(queue, bufC, CL_TRUE, 0, numRowsMatrixA * SIZE * sizeof(int), matrixCPortion, 0, NULL, NULL);

		// Configure the myGEMM kernel and set its arguments
		cl_kernel kernel = clCreateKernel(program, "matrixMultiplication", NULL);
		clSetKernelArg(kernel, 0, sizeof(int), (void*)&M);
		clSetKernelArg(kernel, 1, sizeof(int), (void*)&N);
		clSetKernelArg(kernel, 2, sizeof(int), (void*)&K);
		clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&bufA);
		clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&bufB);
		clSetKernelArg(kernel, 5, sizeof(cl_mem), (void*)&bufC);

		const size_t local[2] = { TS, TS };
		const size_t global[2] = { M, N };
		clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, local, 0, NULL, &event);

		// Wait for calculations to be finished
		clWaitForEvents(1, &event);

		// Copy the output matrix C back to the CPU memory
		clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, numRowsMatrixA * SIZE * sizeof(int), matrixCPortion, 0, NULL, NULL);
		clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, SIZE * SIZE * sizeof(int), matrixC, 0, NULL, NULL);

	
		MPI_Send(&startingRow, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&numRowsMatrixA, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&matrixCPortion, numRowsMatrixA*SIZE, MPI_INT, 0, 1, MPI_COMM_WORLD);

		// Free the OpenCL memory objects
		clReleaseMemObject(bufA);
		clReleaseMemObject(bufB);
		clReleaseMemObject(bufC);
		clReleaseKernel(kernel);
	}

	return 0;
}

void initialiseMatrices()
{
	srand(time(NULL));

	// Initialiase the 3 matrices and fill the 2 to be multiplied with random numbers up to 9
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			matrixA[i][j] = rand() % 9 + 1;
			matrixB[i][j] = rand() % 9 + 1;
			matrixC[i][j] = 0;
		}
	}
}

void printMatrixC()
{
	std::cout << "\nOutput Matrix:\n";
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			std::cout << matrixC[i][j] << " ";
		}
		std::cout << "\n";
	}
}
