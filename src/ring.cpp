#include "mpi_ring.hpp"

#include <mpi.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
using Buffer = std::vector<unsigned char>;

void append_bytes(Buffer &out, const void *data, std::size_t byte_count) {
  const auto *first = static_cast<const unsigned char *>(data);
  out.insert(out.end(), first, first + byte_count);
}

template <typename T> void append_value(Buffer &out, const T &value) {
  static_assert(std::is_trivially_copyable_v<T>);
  append_bytes(out, &value, sizeof(T));
}

mpi_experiments::RingMetrics benchmark_ring_buffer(const std::string &label,
                                                   const void *payload,
                                                   std::size_t payload_bytes,
                                                   int rounds, MPI_Comm comm) {
  if (payload_bytes > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error(
        "Payload too large for MPI_Send / MPI_Recv int count");

  int rank = 0;
  int size = 0;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  std::vector<unsigned char> recv_buffer(payload_bytes, 0U);
  const int byte_count = static_cast<int>(payload_bytes);

  MPI_Barrier(comm);
  double seconds = -MPI_Wtime();

  for (int round = 0; round < rounds; ++round) {
    if (rank == 0) {
      MPI_Send(payload, byte_count, MPI_BYTE, 1, 0, comm);
      MPI_Recv(recv_buffer.data(), byte_count, MPI_BYTE, size - 1, 0, comm,
               MPI_STATUS_IGNORE);
    } else {
      MPI_Recv(recv_buffer.data(), byte_count, MPI_BYTE, rank - 1, 0, comm,
               MPI_STATUS_IGNORE);
      const int next = (rank + 1) % size;
      MPI_Send(recv_buffer.data(), byte_count, MPI_BYTE, next, 0, comm);
    }
  }

  MPI_Barrier(comm);
  seconds += MPI_Wtime();

  mpi_experiments::RingMetrics metrics{};
  metrics.label = label;
  metrics.rounds = rounds;
  metrics.processes = size;
  metrics.payload_bytes = payload_bytes;
  metrics.seconds = seconds;
  metrics.latency_us_per_hop =
      (seconds * 1e6) / (static_cast<double>(rounds) * size);

  const double total_bytes = static_cast<double>(payload_bytes) * rounds * size;
  metrics.bandwidth_mib_s = total_bytes / (seconds * 1024.0 * 1024.0);
  return metrics;
}

} // namespace

namespace mpi_experiments {
HeatDiffusionPatch
HeatDiffusionPatch::make_demo(std::size_t node_count,
                              std::size_t stiffness_entries) {
  HeatDiffusionPatch patch{};
  patch.material.conductivity = 16.75;
  patch.material.density = 7850.0;
  patch.material.specific_heat = 502.0;
  patch.time_step_seconds = 1.0e-4;
  patch.ambient_kelvin = 293.15;
  patch.cell_width_m = 0.025;

  patch.nodal_temperatures.resize(node_count);
  for (std::size_t i = 0; i < node_count; ++i)
    patch.nodal_temperatures[i] =
        patch.ambient_kelvin + 0.01 * static_cast<double>(i % 97U);

  patch.local_stiffness_matrix.resize(stiffness_entries);
  for (std::size_t i = 0; i < stiffness_entries; ++i)
    patch.local_stiffness_matrix[i] = 1.0 / static_cast<double>((i % 31U) + 1U);

  return patch;
}

std::vector<unsigned char> HeatDiffusionPatch::serialize() const {
  Buffer out;
  out.reserve(sizeof(material) + 3U * sizeof(double) +
              2U * sizeof(std::uint64_t) +
              nodal_temperatures.size() * sizeof(double) +
              local_stiffness_matrix.size() * sizeof(double));

  append_value(out, material);
  append_value(out, time_step_seconds);
  append_value(out, ambient_kelvin);
  append_value(out, cell_width_m);

  const auto node_count = static_cast<std::uint64_t>(nodal_temperatures.size());
  append_value(out, node_count);
  if (!nodal_temperatures.empty())
    append_bytes(out, nodal_temperatures.data(),
                 nodal_temperatures.size() * sizeof(double));

  const auto stiffness_count =
      static_cast<std::uint64_t>(local_stiffness_matrix.size());
  append_value(out, stiffness_count);
  if (!local_stiffness_matrix.empty())
    append_bytes(out, local_stiffness_matrix.data(),
                 local_stiffness_matrix.size() * sizeof(double));

  return out;
}

RingMetrics benchmark_ring_int(int rounds, MPI_Comm comm) {
  const int token = 42;
  return benchmark_ring_buffer("int32 token", &token, sizeof(token), rounds,
                               comm);
}

RingMetrics benchmark_ring_double(int rounds, MPI_Comm comm) {
  const double temperature = 273.15;
  return benchmark_ring_buffer("double scalar", &temperature,
                               sizeof(temperature), rounds, comm);
}

RingMetrics benchmark_ring_small_struct(int rounds, MPI_Comm comm) {
  const ParticleCellState particle{
      .x_coordinate = 1.25,
      .y_coordinate = -0.75,
      .temperature = 612.4,
      .residual = 3.0e-6,
      .element_id = 17,
      .level = 2,
  };

  static_assert(std::is_trivially_copyable_v<ParticleCellState>);
  return benchmark_ring_buffer("small POD struct", &particle, sizeof(particle),
                               rounds, comm);
}

RingMetrics benchmark_ring_fem_patch(int rounds, std::size_t node_count,
                                     std::size_t stiffness_entries,
                                     MPI_Comm comm) {
  const auto patch =
      HeatDiffusionPatch::make_demo(node_count, stiffness_entries);
  const auto payload = patch.serialize();
  return benchmark_ring_buffer("FEM heat patch", payload.data(), payload.size(),
                               rounds, comm);
}

} // namespace mpi_experiments
