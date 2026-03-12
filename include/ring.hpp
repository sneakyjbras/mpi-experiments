#pragma once

#include <mpi.h>

namespace mpi_experiments {
struct RingStats {
  int rounds = 0;
  int processes = 0;
  double seconds = 0.0;
  double avg_us_per_sendrecv = 0.0;
};

RingStats run_ring_sendrecv(int rounds, MPI_Comm comm = MPI_COMM_WORLD);

} // namespace mpi_experiments
