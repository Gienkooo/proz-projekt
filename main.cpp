#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <mpi.h>

#include "types.h"
#include "ProcessLogic.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 1)
    {
        if (world_rank == 0)
        {
            std::cerr << "Error: Must be run with at least one MPI process." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    ProcessLogic process_logic(world_rank + 1, world_rank,
                               world_size, D_HOUSES_DEFAULT, P_PASERS_DEFAULT);

    process_logic.run();

    std::cout << "[P" << (world_rank + 1) << "] ProcessLogic::run() completed. Finalizing MPI." << std::endl;

    MPI_Finalize();
    return 0;
}
