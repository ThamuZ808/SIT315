#define HAVE_STRUCT_TIMESPEC
#include "pthread.h"
#include "iostream"
#include "chrono"
#include "omp.h"

using namespace std;

const int arrayX = 200;
const int arrayY = 200;

int matrixA[arrayX][arrayY];
int matrixB[arrayX][arrayY];
int matrixC[arrayX][arrayY];

pthread_t threads[arrayX];

void* ThreadMultiply(void* threadid)
{
	int i = (int)threadid;

	for (int j = 0; j < arrayY; ++j)
		for (int k = 0; k < arrayY; ++k)
		{
			matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
		}

	return NULL;
}

int main()
{
	srand(time(NULL));

	for (int i = 0; i < arrayX; ++i)
	{
		for (int j = 0; j < arrayY; ++j)
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
	for (int i = 0; i < arrayX; ++i)
	{
		pthread_create(&threads[i], NULL, &ThreadMultiply, (void *)i);
	}

	auto end = chrono::steady_clock::now();

	cout << "Elapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";

	cout << "Output Matrix:\n";
	for (int i = 0; i < arrayX; ++i)
	{
		for (int j = 0; j < arrayY; ++j)
		{
			cout << matrixC[i][j] << " ";
		}
		cout << "\n";
	}

	cin;

	return 0;
}