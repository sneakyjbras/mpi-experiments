#pragma once

#include <mpi.h>

#include <string>

namespace mpi_experiments {
class MpiEnv {
public:
  MpiEnv(int &argc, char **&argv);
  ~MpiEnv();

  MpiEnv(const MpiEnv &) = delete;
  MpiEnv &operator=(const MpiEnv &) = delete;
};

std::string get_node_name();
void send_string(const std::string &message, int destination, int tag,
                 MPI_Comm comm = MPI_COMM_WORLD);
std::string recv_string(int source, int tag, MPI_Comm comm = MPI_COMM_WORLD);

} // namespace mpi_experiments
