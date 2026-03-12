#include "mpi_broadcast.hpp"

#include <mpi.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
std::vector<double> make_large_array(std::size_t element_count)
{
  std::vector<double> values(element_count);
  for (std::size_t i = 0; i < element_count; ++i)
    values[i] = 0.125 * static_cast<double>((i % 97U) + 1U);
  return values;
}

mpi_experiments::BroadcastMetrics finalize_metrics(const std::string& label,
                                                   int                rounds,
                                                   int                processes,
                                                   std::size_t        payload_bytes,
                                                   double             local_seconds,
                                                   MPI_Comm           comm)
{
  double seconds = 0.0;
  MPI_Allreduce(&local_seconds, &seconds, 1, MPI_DOUBLE, MPI_MAX, comm);

  mpi_experiments::BroadcastMetrics metrics{};
  metrics.label                = label;
  metrics.rounds               = rounds;
  metrics.processes            = processes;
  metrics.payload_bytes        = payload_bytes;
  metrics.seconds              = seconds;
  metrics.latency_us_per_bcast = (seconds * 1e6) / static_cast<double>(rounds);

  const double total_bytes = static_cast<double>(payload_bytes) * static_cast<double>(rounds)
                             * static_cast<double>(processes - 1);
  metrics.bandwidth_mib_s = total_bytes / (seconds * 1024.0 * 1024.0);
  return metrics;
}

} // namespace

namespace mpi_experiments
{
BroadcastMetrics
benchmark_mpi_bcast_large_array(int rounds, std::size_t element_count, MPI_Comm comm)
{
  if (element_count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error("Element count too large for MPI_Bcast int count");

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  auto              values            = make_large_array(element_count);
  const int         element_count_int = static_cast<int>(element_count);
  const std::size_t payload_bytes     = element_count * sizeof(double);

  MPI_Barrier(comm);
  const double start = MPI_Wtime();

  for (int round = 0; round < rounds; ++round)
  {
    if (rank == 0 && !values.empty())
      values[0] = 1000.0 + static_cast<double>(round);

    MPI_Bcast(values.data(), element_count_int, MPI_DOUBLE, 0, comm);
  }

  const double stop = MPI_Wtime();
  MPI_Barrier(comm);

  return finalize_metrics("MPI_Bcast array", rounds, size, payload_bytes, stop - start, comm);
}

BroadcastMetrics
benchmark_manual_broadcast_large_array(int rounds, std::size_t element_count, MPI_Comm comm)
{
  if (element_count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error("Element count too large for MPI_Send / MPI_Recv int count");

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  auto              values            = make_large_array(element_count);
  const int         element_count_int = static_cast<int>(element_count);
  const std::size_t payload_bytes     = element_count * sizeof(double);

  MPI_Barrier(comm);
  const double start = MPI_Wtime();

  for (int round = 0; round < rounds; ++round)
  {
    if (rank == 0)
    {
      if (!values.empty())
        values[0] = 2000.0 + static_cast<double>(round);

      for (int destination = 1; destination < size; ++destination)
        MPI_Send(values.data(), element_count_int, MPI_DOUBLE, destination, 300, comm);
    }
    else
    {
      MPI_Recv(values.data(), element_count_int, MPI_DOUBLE, 0, 300, comm, MPI_STATUS_IGNORE);
    }
  }

  const double stop = MPI_Wtime();
  MPI_Barrier(comm);

  return finalize_metrics("Manual Send/Recv", rounds, size, payload_bytes, stop - start, comm);
}

} // namespace mpi_experiments
