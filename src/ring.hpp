#pragma once

#include <mpi.h>

namespace sendrecv
{
struct Stats
{
  int    rounds              = 0;
  int    processes           = 0;
  double seconds             = 0.0;
  double avg_us_per_sendrecv = 0.0;
};

Stats run_ring_sendrecv(int rounds, MPI_Comm comm = MPI_COMM_WORLD);

} // namespace sendrecv
