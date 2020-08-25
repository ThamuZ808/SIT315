#include <iostream>
#include <chrono>
#include <omp.h>
#include <mpi.h>

using namespace std;

const int arrayX = 15;
const int arrayY = 15;

int matrixA[arrayY][arrayX];
int matrixB[arrayY][arrayX];
int matrixC[arrayY][arrayX];

MPI_Status mpiStatus;

void initialiseMatrices();
void printMatrixC();

int main(int argc, char *argv[])
{
	int numTasks, destID, tag = 1;
	const int numSlaves = 5;

	MPI_Init(&argc, &argv);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numTasks);

	/* send matrix data to the worker tasks */
	int numRowsMatrixA;
	int startingRow = 0;

	int matrixAPortion[(arrayY / numSlaves) + (arrayY % numSlaves)][arrayX];
	int matrixCPortion[(arrayY / numSlaves) + (arrayY % numSlaves)][arrayX];

	// Fill matrixCPortion with 0's
	for (int i = 0; i < (arrayY / numSlaves) + (arrayY % numSlaves); i++)
		for (int j = 0; j < arrayX; j++)
			matrixCPortion[i][j] = 0;

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
			MPI_Send(&matrixB, arrayX*arrayY, MPI_INT, destID, 0, MPI_COMM_WORLD);

			// First slave gets any extra rows if dividing the rows does not equal a whole number.
			if (destID == 1)
			{
				numRowsMatrixA = (arrayY / numSlaves) + (arrayY % numSlaves);
				
				// Send number of rows
				MPI_Send(&numRowsMatrixA, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

				// Fill the portion with the segment of matrixA
				for (int i = 0; i < numRowsMatrixA; i++)
					for (int j = 0; j < arrayX; j++)
						matrixAPortion[i][j] = matrixA[startingRow + i][j];
				
				// Send chunk of matrixA
				MPI_Send(&matrixAPortion, numRowsMatrixA*arrayX, MPI_INT, destID, 0, MPI_COMM_WORLD);

				startingRow += numRowsMatrixA;
			}
			else
			{
				numRowsMatrixA = arrayY / numSlaves;

				// Send number of rows
				MPI_Send(&numRowsMatrixA, 1, MPI_INT, destID, 0, MPI_COMM_WORLD);

				// Fill the portion with the segment of matrixA
				for (int i = 0; i < numRowsMatrixA; i++)
					for (int j = 0; j < arrayX; j++)
						matrixAPortion[i][j] = matrixA[startingRow + i][j];

				// Send chunk of matrixA
				MPI_Send(&matrixAPortion, numRowsMatrixA*arrayX, MPI_INT, destID, 0, MPI_COMM_WORLD);

				startingRow += numRowsMatrixA;
			}
		}

		// wait for results from slaves
		for (int i = 1; i <= numSlaves; i++)
		{
			MPI_Recv(&startingRow, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);
			MPI_Recv(&numRowsMatrixA, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);
			MPI_Recv(&matrixCPortion, numRowsMatrixA*arrayX, MPI_INT, i, 1, MPI_COMM_WORLD, &mpiStatus);

			int counter = 0;
			for (int j = startingRow; j < (startingRow + numRowsMatrixA); j++)
			{
				for (int k = 0; k < arrayX; k++)
					matrixC[j][k] = matrixCPortion[counter][k];
				counter++;
			}
		}

		printMatrixC();
		end = chrono::steady_clock::now();
	}

	//////////////////////////////////////////////////
	// Slave Receiving
	//////////////////////////////////////////////////
	if (rank > 0)
	{
		// Receive starting row for this slave
		MPI_Recv(&startingRow, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive full matrixB
		MPI_Recv(&matrixB, arrayY*arrayX, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive row count
		MPI_Recv(&numRowsMatrixA, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);
		// Receive portion of matrixA
	    MPI_Recv(&matrixAPortion, numRowsMatrixA*arrayX, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpiStatus);

		//#pragma omp parallel for private(i,j,k) shared(matrixAPortion,matrixB,matrixCPortion)
		for (int i = 0; i < numRowsMatrixA; i++)
			for (int j = 0; j < arrayX; j++)
				for (int k = 0; k < arrayX; k++)
					matrixCPortion[i][j] += matrixAPortion[i][k] * matrixB[k][j];

		MPI_Send(&startingRow, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&numRowsMatrixA, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		MPI_Send(&matrixCPortion, numRowsMatrixA*arrayX, MPI_INT, 0, 1, MPI_COMM_WORLD);
	}

	// Print elapsed time
	cout << "\nElapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";
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

void printMatrixC()
{
	cout << "\nOutput Matrix:\n";
	for (int i = 0; i < arrayY; ++i)
	{
		for (int j = 0; j < arrayX; ++j)
		{
			cout << matrixC[i][j] << " ";
		}
		cout << "\n";
	}
}
