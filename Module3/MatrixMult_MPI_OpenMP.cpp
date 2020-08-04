#include <iostream>
#include <chrono>
#include <omp.h>
#include <mpi.h>

using namespace std;

const int arrayX = 10;
const int arrayY = 10;

int matrixA[arrayY][arrayX];
int matrixB[arrayY][arrayX];
int matrixC[arrayY][arrayX];

MPI_Status mpiStatus;

void initialiseMatrices();
void printResultMatrix();

int main(int argc, char *argv[])
{
	int numTasks, destID, tag = 1;
	const int numSlaves = 3;

	MPI_Init(&argc, &argv);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numTasks);

	/* send matrix data to the worker tasks */
	const int numRowsPerSlave = arrayY / numSlaves;
	const int numRowsForFirstSlave = numRowsPerSlave + (arrayY % numSlaves);
	int startingRow = 0;

	chrono::time_point<chrono::steady_clock> start;
	chrono::time_point<chrono::steady_clock> end;

	//////////////////////////////////////////////////
	// Master sending
	//////////////////////////////////////////////////

	if (rank == 0)
	{
		initialiseMatrices();

		// Begin clock and start matrix multiplication
		start = chrono::steady_clock::now();

		for (destID = 1; destID <= numSlaves; destID++)
		{
			// Send starting point in matrixA
			MPI_Send(&startingRow, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

			// Send all of matrixB to slave.

			MPI_Send(&matrixB, arrayX*arrayY, MPI_DOUBLE, destID, 0, MPI_COMM_WORLD);

			// Send empty matrixC to slave.
			MPI_Send(&matrixC, arrayX*arrayY, MPI_DOUBLE, destID, 0, MPI_COMM_WORLD);

			// First slave gets any extra rows if dividing the rows does not equal a whole number.
			if (destID == 1)
			{
				const int rowsForThisSlave = numRowsForFirstSlave;
				int rowsToSend[rowsForThisSlave][arrayX];

				for (int i = startingRow; i < (i + rowsForThisSlave); i++)
					for (int j = 0; j < arrayX; j++)
						rowsToSend[i][j] = matrixA[i][j];

				// Send number of rows
				MPI_Send(&rowsForThisSlave, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);
				// Send chunk of matrixA
				MPI_Send(&rowsToSend, rowsForThisSlave*arrayX, MPI_DOUBLE, destID, 0, MPI_COMM_WORLD);

				startingRow += rowsForThisSlave;
			}
			else
			{
				const int rowsForThisSlave = numRowsPerSlave;
				int rowsToSend[rowsForThisSlave][arrayX];

				for (int i = startingRow; i < (i + rowsForThisSlave); i++)
					for (int j = 0; j < arrayX; j++)
						rowsToSend[i][j] = matrixA[i][j];

				// Send number of rows
				MPI_Send(&rowsForThisSlave, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);
				// Send chunk of matrixA
				MPI_Send(&rowsToSend, rowsForThisSlave*arrayX, MPI_DOUBLE, destID, 0, MPI_COMM_WORLD);

				startingRow += rowsForThisSlave;
			}
		}

		// wait for results from slaves
		for (int i = 1; i <= numSlaves; i++)
		{
			int source = i;
			int rows;

			MPI_Recv(&startingRow, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &mpiStatus);
			MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &mpiStatus);

			int** tempMatrixC = new int*[arrayY];
			for (int i = 0; i < arrayY; ++i)
				tempMatrixC[i] = matrixC[i];

			MPI_Recv(&matrixC, rows*arrayX, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &mpiStatus);

			for (int i = startingRow; i < (startingRow + rows); i++)
				tempMatrixC[i] = matrixC[i];

			for (int i = 0; i < arrayY; i++)
				for (int j = 0; j < arrayX; j++)
					matrixC[i][j] = tempMatrixC[i][j];

			for (int i = 0; i < rows; ++i)
				delete tempMatrixC[i];
			delete tempMatrixC;
		}

		end = chrono::steady_clock::now();
	}

	//////////////////////////////////////////////////
	// Slave Receiving
	//////////////////////////////////////////////////
	if (rank > 0)
	{
		int source = 0;
		int rows;

		MPI_Recv(&startingRow, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &mpiStatus);
		MPI_Recv(&matrixB, arrayY*arrayX, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &mpiStatus);

		MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &mpiStatus);
		MPI_Recv(&matrixA, rows*arrayX, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &mpiStatus);
		MPI_Recv(&matrixC, arrayY*arrayX, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &mpiStatus);

		/* Matrix multiplication */
		for (int i = startingRow; i < (startingRow + rows); i++)
			for (int j = 0; j < arrayX; j++)
				for (int k = 0; k < arrayX; k++)
					matrixC[i][j] += matrixA[i][k] * matrixB[k][j];


		MPI_Send(&startingRow, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&rows, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&matrixC, rows*arrayX, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
	}

	// Print elapsed time
	cout << "Elapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";

	printResultMatrix();
	cin;

	MPI_Finalize();

	return 0;
}

void initialiseMatrices()
{
	srand(time(NULL));

	// Initialiase the 3 matrices and fill the 2 to be multiplied with random numbers up to 9
	for (int i = 0; i < arrayY; ++i)
	{
		for (int j = 0; j < arrayX; ++j)
		{
			matrixA[i][j] = rand() % 9 + 1;
			matrixB[i][j] = rand() % 9 + 1;
			matrixC[i][j] = 0;
		}
	}
}

void printResultMatrix()
{
	cout << "Output Matrix:\n";
	for (int i = 0; i < arrayY; ++i)
	{
		for (int j = 0; j < arrayX; ++j)
		{
			cout << matrixC[i][j] << " ";
		}
		cout << "\n";
	}
}

/*
///////////
// OpenMP
///////////
#pragma omp parallel for private(i,j,k) shared(matrixA,matrixB,matrixC)
for (int i = 0; i < arrayX; ++i)
for (int j = 0; j < arrayY; ++j)
for (int k = 0; k < arrayY; ++k)
{
matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
}
*/
