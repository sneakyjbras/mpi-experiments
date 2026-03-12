#include "mpi_utils.hpp"

#include <vector>

namespace mpi_experiments {
MpiEnv::MpiEnv(int &argc, char **&argv) { MPI_Init(&argc, &argv); }

MpiEnv::~MpiEnv() { MPI_Finalize(); }

std::string get_node_name() {
  std::vector<char> name(static_cast<std::size_t>(MPI_MAX_PROCESSOR_NAME),
                         '\0');
  int len = 0;
  MPI_Get_processor_name(name.data(), &len);
  return std::string(name.data(), static_cast<std::size_t>(len));
}

void send_string(const std::string &message, int destination, int tag,
                 MPI_Comm comm) {
  const int size = static_cast<int>(message.size());
  MPI_Send(&size, 1, MPI_INT, destination, tag, comm);

  if (size > 0)
    MPI_Send(message.data(), size, MPI_CHAR, destination, tag + 1, comm);
}

std::string recv_string(int source, int tag, MPI_Comm comm) {
  int size = 0;
  MPI_Recv(&size, 1, MPI_INT, source, tag, comm, MPI_STATUS_IGNORE);

  std::string message(static_cast<std::size_t>(size), '\0');
  if (size > 0)
    MPI_Recv(message.data(), size, MPI_CHAR, source, tag + 1, comm,
             MPI_STATUS_IGNORE);

  return message;
}

} // namespace mpi_experiments
