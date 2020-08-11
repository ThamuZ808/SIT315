// Quicksort.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include "chrono"

using namespace std;

const int arraySize = 100000;

int intArray[arraySize];

void displayArray();
void initializeArray();
void quicksort(int low, int high);
int partition(int start, int end);
void swap(int* a, int* b);

int main()
{
	srand(time(NULL));
	initializeArray();
	//displayArray();
	auto start = chrono::steady_clock::now();
	#pragma omp parallel default(none) shared(intArray, arraySize)
	{
		#pragma omp single nowait
		quicksort(0, arraySize - 1);
	}
	auto end = chrono::steady_clock::now();
	//displayArray();
	cout << "Elapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";
}

void quicksort(int low, int high)
{
	if (low < high)
	{
		int partIndex = partition(low, high);
		#pragma omp task shared (intArray) private (low,partIndex)
		quicksort(low, partIndex - 1);
		#pragma omp task shared (intArray) private (hight,partIndex)
		quicksort(partIndex + 1, high);
		#pragma omp taskwait
	}
}

int partition(int start, int end)
{
	int pivot = intArray[end];
	int pivotIndex = start;

	for (int i = start; i < end; i++)
	{
		if (intArray[i] <= pivot)
		{
			swap(&intArray[i], &intArray[pivotIndex]);
			pivotIndex++;
		}
	}

	swap(&intArray[end], &intArray[pivotIndex]);
	return pivotIndex;
}

void swap(int* a, int* b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void displayArray()
{
	cout << "Array:\n";
	for (int i = 0; i < arraySize; i++)
	{
		cout << intArray[i] << " ";
	}
	cout << "\n";
}

void initializeArray()
{
	for (int i = 0; i < arraySize; i++)
		intArray[i] = rand() % arraySize + 1;
}
