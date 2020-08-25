#include "iostream"
#include "chrono"
#include "omp.h"

using namespace std;

#define MAX_THREADS 3
#define MAX 10

int matrixA[MAX][MAX];
int matrixB[MAX][MAX];
int matrixC[MAX][MAX];

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

	///////////
	// OpenMP
	///////////
	int i, j, k;
	#pragma omp parallel for private(i,j,k) shared(matrixA,matrixB,matrixC)	num_threads(MAX_THREADS)
	for (i = 0; i < MAX; ++i)
		for (j = 0; j < MAX; ++j)
			for (k = 0; k < MAX; ++k)
			{
				matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
			}

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
