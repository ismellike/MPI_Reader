#include <iostream>
#include <string>
#include <mpi.h>
#include "AcmeReader.h"

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	int rank, n;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &n);

	if (argc != 4)
	{
		if (rank == 0)
		{
			std::cout << "Usage: mpirun ... reportGenerator dataPath reportYear customerType" << std::endl;
			MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
		}
		return -1;
	}

	char type = argv[3][0];
	int year = std::stoi(argv[2]);
	std::string path = argv[1];
	AcmeReader reader(rank, path, year, type);
	reader.Run();

	MPI_Finalize();

	return 0;
}