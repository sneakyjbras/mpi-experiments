#include "mpi_greetings.hpp"

#include "mpi_utils.hpp"

#include <iostream>
#include <string>

namespace mpi_experiments {
int run_greetings(MPI_Comm comm) {
  int rank = 0;
  int size = 0;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (size < 2) {
    if (rank == 0)
      std::cerr << "This program requires at least 2 MPI processes.\n";
    return 1;
  }

  const std::string node_name = get_node_name();

  if (rank == 0) {
    std::cout << "Task 0 is receiving greetings on machine " << node_name
              << "\n";

    for (int source = 1; source < size; ++source) {
      const std::string greeting = recv_string(source, 100, comm);
      std::cout << "Task 0 received: " << greeting << "\n";

      const std::string reply = "Task 0 sends greetings back from machine " +
                                node_name + " to task " +
                                std::to_string(source);
      send_string(reply, source, 200, comm);
    }
  } else {
    const std::string greeting = "Task " + std::to_string(rank) +
                                 " sends greetings from machine " + node_name;
    send_string(greeting, 0, 100, comm);

    const std::string reply = recv_string(0, 200, comm);
    std::cout << "Task " << rank << " received reply: " << reply << "\n";
  }

  return 0;
}

} // namespace mpi_experiments
