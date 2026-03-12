#include "mpi_greetings.hpp"
#include "mpi_utils.hpp"

int main(int argc, char **argv) {
  mpi_experiments::MpiEnv env(argc, argv);
  return mpi_experiments::run_greetings();
}
