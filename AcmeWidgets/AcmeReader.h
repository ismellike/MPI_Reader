#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <mpi.h>

struct Widget {
	int modelNumber;
	int purchaseDate;
	char customerType;
	float purchaseAmount;
};

class AcmeReader
{
public:
	AcmeReader(int rank, std::string path, int reportYear, char customerType);
	void Run();
	void rank0();
	void rank1();
	void rankN();
	bool ReadFile();
	void getWidgetMPI(MPI_Datatype* widgetMPI);
	void getSubComm(MPI_Comm* subComm, int exclusion = 1);
	int getCode(Widget* widget, int& year);
	void HandleSection(Widget* widgetPartition, int size);
	void DisplayResults(float* finalTotals[3]);
	~AcmeReader();
private:
	Widget* widgets = nullptr;
	MPI_Datatype widgetMPI;
	int numRecords, numModels;
	int Rank;
	std::string Path;
	int ReportYear;
	char CustomerType;
	float* totals = nullptr;
	float* totalsYear = nullptr;
	float* totalsType = nullptr;
};

