#include "mpi_broadcast.hpp"
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

  const auto [ptr, ec] = std::from_chars(begin, end, value, 10);
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

  const auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr != end || value == 0)
    return false;

  out = static_cast<std::size_t>(value);
  return true;
}

void print_usage(const char* program)
{
  std::cerr << "Usage: " << program << " <n-rounds> [array-elements]\n";
}

} // namespace

int main(int argc, char** argv)
{
  mpi_experiments::MpiEnv env(argc, argv);

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc < 2 || argc > 3)
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

  std::size_t element_count = 1U << 20U;
  if (argc == 3 && !parse_positive_size(argv[2], element_count))
  {
    if (rank == 0)
      std::cerr << "Invalid [array-elements]: \"" << argv[2]
                << "\" (expected a positive integer)\n";
    return 1;
  }

  std::vector<mpi_experiments::BroadcastMetrics> results;
  results.push_back(
      mpi_experiments::benchmark_mpi_bcast_large_array(rounds, element_count, MPI_COMM_WORLD));
  results.push_back(mpi_experiments::benchmark_manual_broadcast_large_array(rounds, element_count,
                                                                            MPI_COMM_WORLD));

  if (rank == 0)
  {
    const auto payload_bytes = element_count * sizeof(double);

    std::cout << "Broadcast MPI micro-benchmarks\n";
    std::cout << "Processes = " << size << ", Rounds = " << rounds << "\n";
    std::cout << "Array size = " << element_count << " doubles (" << payload_bytes << " bytes)\n\n";

    std::cout << std::left << std::setw(20) << "Method" << std::right << std::setw(12) << "Bytes"
              << std::setw(14) << "Time (s)" << std::setw(22) << "Latency (us/bcast)"
              << std::setw(22) << "Bandwidth (MiB/s)" << '\n';
    std::cout << std::string(90, '-') << '\n';

    for (const auto& metrics : results)
    {
      std::cout << std::left << std::setw(20) << metrics.label << std::right << std::setw(12)
                << metrics.payload_bytes << std::setw(14) << std::fixed << std::setprecision(6)
                << metrics.seconds << std::setw(22) << std::fixed << std::setprecision(3)
                << metrics.latency_us_per_bcast << std::setw(22) << std::fixed
                << std::setprecision(3) << metrics.bandwidth_mib_s << '\n';
    }

    std::cout << "\nDefinitions:\n";
    std::cout << "- Latency is the average time per broadcast operation.\n";
    std::cout << "- Bandwidth is the effective application-level throughput:\n";
    std::cout << "  payload_bytes * rounds * (processes - 1) / total_time.\n";
    std::cout << "- MPI_Bcast is the usual optimized collective implementation, while\n";
    std::cout << "  Manual Send/Recv uses rank 0 as a simple explicit broadcaster.\n";
  }

  return 0;
}
