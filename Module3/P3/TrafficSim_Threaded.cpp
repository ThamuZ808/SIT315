#include <iostream>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <time.h>
#include <queue>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

using namespace std;

// This only matters if you are generating new data
#define NUM_TRAFFIC_LIGHTS 5
#define NUM_SIMULATED_HOURS 12

// Both combined equals number of threads to use.
#define NUM_PRODUCERS 4 // This is also number of traffic lights simulated
#define NUM_CONSUMERS 2

class signalData
{
public:
	string timestamp;
	int lightID;
	int numCars;

	signalData(string timestamp, int lightID, int numCars);
};

signalData::signalData(string timestamp, int lightID, int numCars)
{
	this->timestamp = timestamp;
	this->lightID = lightID;
	this->numCars = numCars;
}

// Generate new data into file
void generateData();
// Read into queue.
void* produceData(void* threadid);
// Read data from queue and sort it into list.
void* consumeData(void* threadid);
// Display final sorted list.
void displaySortedList();

queue<signalData> dataQueue;
vector<signalData> sortedList;

// Array of threads, max recommended is 6.
pthread_t threads[NUM_PRODUCERS + NUM_CONSUMERS];

// declaring mutex 
pthread_mutex_t queueLock;
vector<string> dataLines;
int lineCount = 0;

void main()
{
	// Run or comment out if you want to generate new data
	generateData();

	// Read data into vector
	ifstream ifs("Test_Data.txt");
	for (string line; getline(ifs, line);)
		dataLines.push_back(line);
	ifs.close();

	pthread_mutex_init(&queueLock, NULL);
	
	int threadCounter = 0;
	for (int i = threadCounter; i < NUM_PRODUCERS; ++i)
	{
		pthread_create(&threads[i], NULL, produceData, (void *)threadCounter);
		threadCounter++;
	}

	for (int i = threadCounter; i < (NUM_PRODUCERS + NUM_CONSUMERS); ++i)
	{
		pthread_create(&threads[i], NULL, consumeData, (void *)threadCounter);
		threadCounter++;
	}

	for (int i = 0; i < (NUM_PRODUCERS + NUM_CONSUMERS); ++i)
		(void)pthread_join(threads[i], NULL);
}

void generateData()
{
	ofstream ofs;
	srand(time(NULL));

	// Clear all data in file and ready it to be written to.
	ofs.open("Test_Data.txt", std::ofstream::out | std::ofstream::trunc);

	int counter = 12;

	// Always starts at midday
	for (int i = 0; i < NUM_SIMULATED_HOURS; i++)
	{
		for (int j = 0; j < 60; j += 5)
		{
			for (int k = 1; k <= NUM_TRAFFIC_LIGHTS; k++)
			{
				if (counter < 10)
				{
					if (j % 60 == 0 || j % 60 == 5)
						ofs << "0" << (0 + counter) << ":0" << (j % 60);
					else
						ofs << "0" << (0 + counter) << ":" << (j % 60);
				}
				else
				{
					if (j % 60 == 0 || j % 60 == 5)
						ofs << (0 + counter) << ":0" << (j % 60);
					else
						ofs << (0 + counter) << ":" << (j % 60);
				}

				ofs << " 0" << k;
				int numCars = rand() % 1000;
				ofs << " " << numCars << "\n";
			}


		}
		if (counter != 23)
			counter++;
		else
			counter = 0;
	}

	ofs.close();
}

void* produceData(void* threadid)
{
	stringstream ss;
	
	while (lineCount < dataLines.size())
	{
		string timestamp = "";
		int lightID = 0;
		int numCars = 0;
		
		// Activate mutex lock, add data to queue, then unlock.
		pthread_mutex_lock(&queueLock);
		
		// Create and add data to queue
		string word = "";
		int counter = 0;

		for (auto x : dataLines[lineCount])
		{
			if (x == ' ')
			{
				if(counter == 0)
					timestamp = word;
				else
					lightID = stoi(word);

				counter++;
				word = "";
			}
			else
				word = word + x;
		}
		numCars = stoi(word);
		
		signalData dataToQueue(timestamp, lightID, numCars);
		dataQueue.push(dataToQueue);
		lineCount++;

		pthread_mutex_unlock(&queueLock);
		this_thread::sleep_for(1s);
	}

	pthread_exit(NULL);
	return NULL;
}

void* consumeData(void* threadid)
{
	int consumerID = (int)threadid;

	while (true)
	{
		pthread_mutex_lock(&queueLock);
		while(!dataQueue.empty())
		{
			string timestamp = dataQueue.front().timestamp;
			int ID = dataQueue.front().lightID;
			int value = dataQueue.front().numCars;

			dataQueue.pop();

			signalData dataToSort(timestamp, ID, value);

			if (!sortedList.empty())
				for (int i = 0; i < sortedList.size(); i++)
				{
					if (sortedList[i].numCars <= value)
					{
						sortedList.insert(sortedList.begin() + i, dataToSort);						
						break;
					}
					else if (i == (sortedList.size() - 1))
					{
						sortedList.push_back(dataToSort);
						break;
					}
				}
			else
			{
				sortedList.push_back(dataToSort);
			}
		}
		displaySortedList();
		pthread_mutex_unlock(&queueLock);

		if (sortedList.size() == dataLines.size())
			break;
		this_thread::sleep_for(3s);
	}

	pthread_exit(NULL);
	return NULL;
}

void displaySortedList()
{
	system("cls");
	cout << "Sorted List:\n";
	for (int i = 0; i < sortedList.size(); i++)
	{
		cout << sortedList[i].timestamp << " " << sortedList[i].lightID << " " << sortedList[i].numCars << "\n";
	}
	cout << "\nSorted list count: " << sortedList.size() << "\n";
}

