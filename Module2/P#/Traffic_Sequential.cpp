#include <iostream>
#include <fstream>
#include <time.h>
#include <queue>
#include <string>

using namespace std;

#define NUM_TRAFFIC_LIGHTS 4
#define NUM_SIMULATED_HOURS 15

// Number of producers and consumers must each be 1 or higher, and both combined must be equal to or less than 6
#define NUM_PRODUCERS 3
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

void generateData();
void produceData();
void consumeData();
void displaySortedList();

queue<signalData> dataQueue;
vector<signalData> sortedList;

void main()
{
	// Sequential solution.
	generateData();	
	produceData();
	consumeData();
	displaySortedList();
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

void produceData()
{
	ifstream ifs;
	ifs.open("Test_Data.txt");

	string time;
	int ID, value;
	while (ifs >> time >> ID >> value)
	{
		signalData dataToQueue(time, ID, value);
		dataQueue.push(dataToQueue);
	}

	ifs.close();
}

void consumeData()
{
	while (!dataQueue.empty())
	{
		string timestamp = dataQueue.front().timestamp;
		int ID = dataQueue.front().lightID;
		int value = dataQueue.front().numCars;

		signalData dataToSort(timestamp, ID, value);
		dataQueue.pop();
		
		if (!sortedList.empty())
			for (int i = 0; i < sortedList.size(); i++)
			{
				if (sortedList[i].numCars <= value)
				{
					sortedList.insert(sortedList.begin() + i, dataToSort);
					break;
				}
				else if (sortedList[i].numCars > value && i == sortedList.size())
				{
					sortedList.insert(sortedList.begin() + i, dataToSort);
					break;
				}
			}
		else
			sortedList.push_back(dataToSort);
	}
}

void displaySortedList()
{
	cout << "Sorted List:\n";
	for (int i = 0; i < sortedList.size(); i++)
	{
		cout << sortedList[i].timestamp << " " << sortedList[i].lightID << " " << sortedList[i].numCars << "\n";
	}
	cout << "\n";
}
