#include "stdafx.h"
#define HAVE_STRUCT_TIMESPEC
#include "pthread.h"
#include "iostream"
#include "chrono"
#include "omp.h"

using namespace std;

#define MAX 5
#define MAX_THREAD 4

int matrixA[MAX][MAX];
int matrixB[MAX][MAX];
int matrixC[MAX][MAX];

pthread_t threads[MAX_THREAD];

int step_i = 0;

void* ThreadMultiply(void* threadid)
{
	int core = step_i++;

	// Each thread computes 1/4th of matrix multiplication 
	for (int i = core * MAX / MAX_THREAD; i < (core + 1) * MAX / MAX_THREAD; i++)
		for (int j = 0; j < MAX; j++)
			for (int k = 0; k < MAX; k++)
				matrixC[i][j] += matrixA[i][k] * matrixB[k][j];

	return NULL;
}

int main()
{
	srand(time(NULL));

	for (int i = 0; i < MAX; ++i)
	{
		for (int j = 0; j < MAX; ++j)
		{
			matrixA[i][j] = rand() % 9 + 1;
			matrixB[i][j] = rand() % 9 + 1;
			matrixC[i][j] = 0;
		}
	}

	// This is the matrix multiplication to be timed
	auto start = chrono::steady_clock::now();

	//////////////
	// Threading
	//////////////
	for (int i = 0; i < MAX_THREAD; ++i)
	{
		int* p = 0;
		pthread_create(&threads[i], NULL, ThreadMultiply, (void *)p);
	}

	for (int i = 0; i < MAX_THREAD; i++)
		pthread_join(threads[i], NULL);

	auto end = chrono::steady_clock::now();

	cout << "Elapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";

	/*cout << "Output Matrix:\n";
	for (int i = 0; i < MAX; ++i)
	{
		for (int j = 0; j < MAX; ++j)
		{
			cout << matrixC[i][j] << " ";
		}
		cout << "\n";
	}*/

	cin;

	return 0;
}
