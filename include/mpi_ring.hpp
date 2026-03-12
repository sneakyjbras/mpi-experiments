#pragma once

#include <mpi.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mpi_experiments
{
struct RingMetrics
{
  std::string label;
  int         rounds             = 0;
  int         processes          = 0;
  std::size_t payload_bytes      = 0;
  double      seconds            = 0.0;
  double      latency_us_per_hop = 0.0;
  double      bandwidth_mib_s    = 0.0;
};

struct CellMaterial
{
  double conductivity  = 0.0;
  double density       = 0.0;
  double specific_heat = 0.0;
};

struct ParticleCellState
{
  double x_coordinate = 0.0;
  double y_coordinate = 0.0;
  double temperature  = 0.0;
  double residual     = 0.0;
  int    element_id   = 0;
  int    level        = 0;
};

class HeatDiffusionPatch
{
public:
  CellMaterial        material{};
  double              time_step_seconds = 0.0;
  double              ambient_kelvin    = 0.0;
  double              cell_width_m      = 0.0;
  std::vector<double> nodal_temperatures;
  std::vector<double> local_stiffness_matrix;

  static HeatDiffusionPatch make_demo(std::size_t node_count, std::size_t stiffness_entries);
  [[nodiscard]] std::vector<unsigned char> serialize() const;
};

RingMetrics benchmark_ring_int(int rounds, MPI_Comm comm = MPI_COMM_WORLD);
RingMetrics benchmark_ring_double(int rounds, MPI_Comm comm = MPI_COMM_WORLD);
RingMetrics benchmark_ring_small_struct(int rounds, MPI_Comm comm = MPI_COMM_WORLD);
RingMetrics benchmark_ring_fem_patch(int         rounds,
                                     std::size_t node_count,
                                     std::size_t stiffness_entries,
                                     MPI_Comm    comm = MPI_COMM_WORLD);

} // namespace mpi_experiments
