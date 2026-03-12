#pragma once

#include <mpi.h>

#include <cstddef>
#include <string>

namespace mpi_experiments
{
struct BroadcastMetrics
{
  std::string label                = {};
  int         rounds               = 0;
  int         processes            = 0;
  std::size_t payload_bytes        = 0;
  double      seconds              = 0.0;
  double      latency_us_per_bcast = 0.0;
  double      bandwidth_mib_s      = 0.0;
};

BroadcastMetrics benchmark_mpi_bcast_large_array(int         rounds,
                                                 std::size_t element_count,
                                                 MPI_Comm    comm = MPI_COMM_WORLD);

BroadcastMetrics benchmark_manual_broadcast_large_array(int         rounds,
                                                        std::size_t element_count,
                                                        MPI_Comm    comm = MPI_COMM_WORLD);

} // namespace mpi_experiments
