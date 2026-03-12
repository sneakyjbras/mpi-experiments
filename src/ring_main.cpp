#include "mpi_ring.hpp"
#include "mpi_utils.hpp"

#include <mpi.h>

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
bool parse_positive_int(std::string_view s, int& out)
{
  if (s.empty())
    return false;

  int         value = 0;
  const char* begin = s.data();
  const char* end   = s.data() + s.size();

  auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr != end || value <= 0)
    return false;

  out = value;
  return true;
}

bool parse_positive_size(std::string_view s, std::size_t& out)
{
  if (s.empty())
    return false;

  std::uint64_t value = 0;
  const char*   begin = s.data();
  const char*   end   = s.data() + s.size();

  auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr != end || value == 0)
    return false;

  out = static_cast<std::size_t>(value);
  return true;
}

void print_usage(const char* program)
{
  std::cerr << "Usage: " << program << " <n-rounds> [fem-node-count] [fem-stiffness-entries]\n";
}

} // namespace

int main(int argc, char** argv)
{
  mpi_experiments::MpiEnv env(argc, argv);

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc < 2 || argc > 4)
  {
    if (rank == 0)
      print_usage(argv[0]);
    return 1;
  }

  if (size < 2)
  {
    if (rank == 0)
      std::cerr << "This program requires at least 2 MPI processes.\n";
    return 1;
  }

  int rounds = 0;
  if (!parse_positive_int(argv[1], rounds))
  {
    if (rank == 0)
      std::cerr << "Invalid <n-rounds>: \"" << argv[1] << "\" (expected a positive integer)\n";
    return 1;
  }

  std::size_t fem_node_count        = 256;
  std::size_t fem_stiffness_entries = 4096;

  if (argc >= 3 && !parse_positive_size(argv[2], fem_node_count))
  {
    if (rank == 0)
      std::cerr << "Invalid [fem-node-count]: \"" << argv[2]
                << "\" (expected a positive integer)\n";
    return 1;
  }

  if (argc == 4 && !parse_positive_size(argv[3], fem_stiffness_entries))
  {
    if (rank == 0)
      std::cerr << "Invalid [fem-stiffness-entries]: \"" << argv[3]
                << "\" (expected a positive integer)\n";
    return 1;
  }

  std::vector<mpi_experiments::RingMetrics> results;
  results.push_back(mpi_experiments::benchmark_ring_int(rounds, MPI_COMM_WORLD));
  results.push_back(mpi_experiments::benchmark_ring_double(rounds, MPI_COMM_WORLD));
  results.push_back(mpi_experiments::benchmark_ring_small_struct(rounds, MPI_COMM_WORLD));
  results.push_back(mpi_experiments::benchmark_ring_fem_patch(
      rounds, fem_node_count, fem_stiffness_entries, MPI_COMM_WORLD));

  if (rank == 0)
  {
    std::cout << "Ring MPI micro-benchmarks\n";
    std::cout << "Processes = " << size << ", Rounds = " << rounds << "\n";
    std::cout << "FEM object = HeatDiffusionPatch with " << fem_node_count << " nodes and "
              << fem_stiffness_entries << " stiffness entries\n\n";

    std::cout << std::left << std::setw(20) << "Payload" << std::right << std::setw(12) << "Bytes"
              << std::setw(14) << "Time (s)" << std::setw(20) << "Latency (us/hop)" << std::setw(22)
              << "Bandwidth (MiB/s)" << '\n';
    std::cout << std::string(88, '-') << '\n';

    for (const auto& metrics : results)
    {
      std::cout << std::left << std::setw(20) << metrics.label << std::right << std::setw(12)
                << metrics.payload_bytes << std::setw(14) << std::fixed << std::setprecision(6)
                << metrics.seconds << std::setw(20) << std::fixed << std::setprecision(3)
                << metrics.latency_us_per_hop << std::setw(22) << std::fixed << std::setprecision(3)
                << metrics.bandwidth_mib_s << '\n';
    }

    std::cout << "\nDefinitions:\n";
    std::cout << "- Latency is the average time for one message hop around the ring.\n";
    std::cout << "- Bandwidth is the effective payload throughput:"
                 " payload_bytes * rounds * processes / total_time.\n";
    std::cout << "- The large FEM object is serialized first because std::vector makes the"
                 " class non-trivial to send directly with raw MPI datatypes.\n";
  }

  return 0;
}
