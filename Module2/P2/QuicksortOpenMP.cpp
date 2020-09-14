// Quicksort.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include "chrono"
#include "omp.h"

using namespace std;

const int arraySize = 1000;

int intArray[arraySize];

void displayArray();
void initializeArray();
void quicksort(int low, int high);
int partition(int start, int end);
void swap(int* a, int* b);

int threadCounter = 0;
int maxThreads = 6;

int main()
{
	srand(time(NULL));
	initializeArray();
	//displayArray();
	auto start = chrono::steady_clock::now();

	omp_set_num_threads(maxThreads);
	omp_set_nested(1);
	quicksort(0, arraySize - 1);

	auto end = chrono::steady_clock::now();
	//displayArray();
	cout << "Elapsed time in nanoseconds: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns\n\n";
}

void quicksort(int low, int high)
{
	
	if (low < high)
	{
		int partIndex = partition(low, high);

		if (threadCounter <= (maxThreads - 2))
		{
			threadCounter += 2;
			#pragma omp parallel sections
			{
				#pragma omp section
				{
					quicksort(low, partIndex - 1);
					threadCounter--;
				}

				#pragma omp section
				{
					quicksort(partIndex + 1, high);
					threadCounter--;
				}
			}
		}
		else
		{
			quicksort(low, partIndex - 1);
			quicksort(partIndex + 1, high);
		}
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
