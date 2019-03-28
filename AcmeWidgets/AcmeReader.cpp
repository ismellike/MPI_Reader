#include "AcmeReader.h"


AcmeReader::AcmeReader(int rank, std::string path, int reportYear, char customerType)
{
	Path = path;
	ReportYear = reportYear;
	CustomerType = customerType;
	Rank = rank;

	getWidgetMPI(&widgetMPI);
}

void AcmeReader::Run()
{
	switch (Rank)
	{
	case 0: //reader
		rank0();
		break;
	case 1: //error reporter
		rank1();
		break;
	default: //workers
		rankN();
		break;
	}
}

void AcmeReader::rank0()
{
	if (!ReadFile())
	{
		std::cout << "An invalid file path was given for reading." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, MPI_ERR_FILE);
		return;
	}

	if (CustomerType != 'I' && CustomerType != 'R' && CustomerType != 'G')
	{
		std::cout << "An invalid customer type was given for reading." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
		return;
	}

	if (ReportYear < 1997 || ReportYear > 2018)
	{
		std::cout << "An invalid report year was given for reading." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
		return;
	}	

	MPI_Comm subComm;
	getSubComm(&subComm);

	int subSize;
	MPI_Comm_size(subComm, &subSize);

	int fileInfo[2] = { numRecords, numModels };
	MPI_Bcast(fileInfo, 2, MPI_INT, 0, subComm);

	int partition = numRecords / subSize;
	int remainder = numRecords % subSize;
	int* sendCounts = new int[subSize];
	int* displacements = new int[subSize];
	displacements[0] = 0;

	for (int i = 0; i < subSize; i++)
	{
		sendCounts[i] = partition;
		if (i < remainder)
			sendCounts[i]++;
	}
	for (int i = 1; i < subSize; i++)
		displacements[i] = displacements[i - 1] + sendCounts[i - 1];

	int sum = 0;
	for (int i = 0; i < subSize; i++)
		sum += sendCounts[i];

	int section = Rank < remainder ? partition + 1 : partition;
	Widget* widgetPartition = new Widget[section];
	MPI_Scatterv(widgets, sendCounts, displacements, widgetMPI, widgetPartition, section, widgetMPI, 0, subComm);

	totals = new float[numModels];
	totalsYear = new float[numModels];
	totalsType = new float[numRecords];
	for (int i = 0; i < numModels; i++)
		totals[i] = totalsYear[i] = totalsType[i] = 0;

	HandleSection(widgetPartition, section);

	//sync up
	MPI_Send(nullptr, 0, widgetMPI, 1, 0, MPI_COMM_WORLD);

	float** finalTotals = new float*[3];
	for (int i = 0; i < 3; i++)
		finalTotals[i] = new float[numModels];

	MPI_Reduce(totals, finalTotals[0], numModels, MPI_FLOAT, MPI_SUM, 0, subComm);
	MPI_Reduce(totalsYear, finalTotals[1], numModels, MPI_FLOAT, MPI_SUM, 0, subComm);
	MPI_Reduce(totalsType, finalTotals[2], numModels, MPI_FLOAT, MPI_SUM, 0, subComm);

	DisplayResults(finalTotals);

	//delete
	delete sendCounts;
	delete widgetPartition;
	delete displacements;
	for (int i = 0; i < 3; i++)
		delete finalTotals[i];
	delete finalTotals;
	delete[] widgets;
	delete totals;
	delete totalsYear;
	delete totalsType;
}

void AcmeReader::rank1()
{
	std::string ErrMessages[6] = { "Bad model number", "Bad purchase date", "Bad purchase month", "Bad purchase year",
		"Bad customer type", "Bad purchase amount" };
	Widget widget;
	MPI_Status status;

	while (true)
	{
		MPI_Recv(&widget, 1, widgetMPI, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		int tag = status.MPI_TAG;

		if (tag == 0)
			break;

		std::cout << "Rank " << status.MPI_SOURCE << " reported \"" << ErrMessages[tag - 1] << "\": {modelNumber: " << widget.modelNumber <<
			", date: " << widget.purchaseDate << ", customerType: " << widget.customerType << ", amount: " << widget.purchaseAmount << '}' << std::endl;
	}
}

void AcmeReader::rankN()
{
	MPI_Comm subComm;
	getSubComm(&subComm);

	int subSize;
	MPI_Comm_size(subComm, &subSize);

	int fileInfo[2];
	MPI_Bcast(fileInfo, 2, MPI_INT, 0, subComm);
	numRecords = fileInfo[0];
	numModels = fileInfo[1];

	int partition = numRecords / subSize;
	int remainder = numRecords % subSize;

	int section = Rank - 1 < remainder ? partition + 1 : partition;
	Widget* widgetPartition = new Widget[section];
	MPI_Scatterv(nullptr, nullptr, nullptr, widgetMPI, widgetPartition, section, widgetMPI, 0, subComm);

	totals = new float[numModels];
	totalsYear = new float[numModels];
	totalsType = new float[numRecords];
	for (int i = 0; i < numModels; i++)
		totals[i] = totalsYear[i] = totalsType[i] = 0;

	HandleSection(widgetPartition, section);

	MPI_Reduce(totals, nullptr, numModels, MPI_FLOAT, MPI_SUM, 0, subComm);
	MPI_Reduce(totalsYear, nullptr, numModels, MPI_FLOAT, MPI_SUM, 0, subComm);
	MPI_Reduce(totalsType, nullptr, numModels, MPI_FLOAT, MPI_SUM, 0, subComm);

	delete widgetPartition;
	delete totals;
	delete totalsType;
	delete totalsYear;
}

bool AcmeReader::ReadFile()
{
	std::ifstream inFile(Path);

	if (!inFile.is_open())
		return false;

	//read first 2 numbers
	inFile >> numRecords >> numModels;

	//make widgets
	widgets = new Widget[numRecords];
	int i = 0;

	while (i < numRecords)
	{
		Widget widget;

		inFile >> widget.modelNumber;
		inFile >> widget.purchaseDate;
		inFile >> widget.customerType;
		inFile >> widget.purchaseAmount;
		//insert
		widgets[i] = widget;
		i++;
	}
	inFile.close();
	return true;
}

void AcmeReader::getWidgetMPI(MPI_Datatype * widgetMPI)
{
	int blockLengths[4] = { 1, 1, 1, 1 };
	MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_CHAR, MPI_FLOAT };
	MPI_Aint displacements[4] = { offsetof(struct Widget, modelNumber), offsetof(struct Widget, purchaseDate), offsetof(struct Widget, customerType), offsetof(struct Widget, purchaseAmount) };
	MPI_Type_create_struct(4, blockLengths, displacements, types, widgetMPI);
	MPI_Type_commit(widgetMPI);
}

void AcmeReader::getSubComm(MPI_Comm * subComm, int exclusion)
{
	const int rank[1] = { exclusion };
	MPI_Group world_group;
	MPI_Comm_group(MPI_COMM_WORLD, &world_group);

	MPI_Group sub_group;
	MPI_Group_excl(world_group, 1, rank, &sub_group);

	MPI_Comm_create_group(MPI_COMM_WORLD, sub_group, 0, subComm);
}

int AcmeReader::getCode(Widget* widget, int& year)
{
	std::string date = std::to_string(widget->purchaseDate);

	if (widget->modelNumber < 0 || widget->modelNumber > numModels)
		return 1;
	if (date.length() != 6)
		return 2;
	
	year = std::stoi(date.substr(0, 4));
	int month = std::stoi(date.substr(4, 6));

	if (month < 1 || month > 12)
		return 3;
	if (year < 1997 || year > 2018)
		return 4;
	if (widget->customerType != 'I' && widget->customerType != 'R' && widget->customerType != 'G')
		return 5;
	if (widget->purchaseAmount <= 0)
		return 6;
	
	return 0;
}

void AcmeReader::HandleSection(Widget * widgetPartition, int size)
{
	for (int i = 0; i < size; i++)
	{
		Widget widget = widgetPartition[i];
		int year;
		int errCode = getCode(&widget, year);

		if (errCode == 0) // no error
		{
			totals[widget.modelNumber] += widget.purchaseAmount;

			if (year == ReportYear)
				totalsYear[widget.modelNumber] += widget.purchaseAmount;

			if (widget.customerType == CustomerType)
				totalsType[widget.modelNumber] += widget.purchaseAmount;
		}
		else //send to rank1
		{
			MPI_Send(&widget, 1, widgetMPI, 1, errCode, MPI_COMM_WORLD);
		}
	}
}

void AcmeReader::DisplayResults(float* finalTotals[3])
{
	std::cout << "Model Number\t\tTotal Amounts\t\tAmounts for " << ReportYear << "\t\tAmounts for Customer Type " << CustomerType << std::endl;
	for (int i = 0; i < numModels; i++)
		std::cout << std::setw(5) << i << std::setw(30) << finalTotals[0][i] << std::setw(25) << finalTotals[1][i] << std::setw(40) << finalTotals[2][i] << std::endl;
}

AcmeReader::~AcmeReader()
{
}
