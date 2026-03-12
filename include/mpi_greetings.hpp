#pragma once

#include <mpi.h>

namespace mpi_experiments {
int run_greetings(MPI_Comm comm = MPI_COMM_WORLD);

} // namespace mpi_experiments
