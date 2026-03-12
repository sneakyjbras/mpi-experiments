#include "ring.hpp"

#include <mpi.h>

#include <cerrno>
#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>

namespace {
struct MpiEnv {
  MpiEnv(int &argc, char **&argv) { MPI_Init(&argc, &argv); }
  ~MpiEnv() { MPI_Finalize(); }

  MpiEnv(const MpiEnv &) = delete;
  MpiEnv &operator=(const MpiEnv &) = delete;
};

bool parse_positive_int(std::string_view s, int &out) {
  if (s.empty())
    return false;

  int v = 0;
  const char *b = s.data();
  const char *e = s.data() + s.size();

  auto [ptr, ec] = std::from_chars(b, e, v, 10);
  if (ec != std::errc{} || ptr != e)
    return false;
  if (v <= 0)
    return false;

  out = v;
  return true;
}
} // namespace

int main(int argc, char **argv) {
  MpiEnv env(argc, argv);

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 2) {
    if (rank == 0)
      std::cerr << "Usage: " << argv[0] << " <n-rounds>\n";
    return 1;
  }

  if (size < 2) {
    if (rank == 0)
      std::cerr << "This program requires at least 2 MPI processes.\n";
    return 1;
  }

  int rounds = 0;
  if (!parse_positive_int(argv[1], rounds)) {
    if (rank == 0)
      std::cerr << "Invalid <n-rounds>: \"" << argv[1]
                << "\" (expected a positive integer)\n";
    return 1;
  }

  const auto stats = sendrecv::run_ring_sendrecv(rounds, MPI_COMM_WORLD);

  if (rank == 0) {
    std::cout << "Rounds= " << stats.rounds
              << ", N Processes = " << stats.processes
              << ", Time = " << std::fixed << std::showpoint;
    std::cout.precision(6);
    std::cout << stats.seconds << " sec,\n";

    std::cout << "Average time per Send/Recv = ";
    std::cout.precision(2);
    std::cout << stats.avg_us_per_sendrecv << " us\n";
  }

  return 0;
}
