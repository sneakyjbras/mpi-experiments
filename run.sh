#!/usr/bin/env bash
set -euo pipefail

NP="${NP:-4}"
ROUNDS="${ROUNDS:-100000}"
FEM_NODES="${FEM_NODES:-256}"
FEM_STIFFNESS="${FEM_STIFFNESS:-4096}"
BCAST_ROUNDS="${BCAST_ROUNDS:-1000}"
BCAST_ELEMENTS="${BCAST_ELEMENTS:-1048576}"
MPI_RUN_FLAGS="${MPI_RUN_FLAGS:-}"
BIN="${BIN:-${1:-}}"

make -j

run_greetings() {
  echo "== Running greetings =="
  echo "mpirun ${MPI_RUN_FLAGS} -np ${NP} ./bin/greetings"
  mpirun ${MPI_RUN_FLAGS} -np "${NP}" ./bin/greetings
}

run_ring() {
  echo "== Running ring =="
  echo "mpirun ${MPI_RUN_FLAGS} -np ${NP} ./bin/ring ${ROUNDS} ${FEM_NODES} ${FEM_STIFFNESS}"
  mpirun ${MPI_RUN_FLAGS} -np "${NP}" ./bin/ring "${ROUNDS}" "${FEM_NODES}" "${FEM_STIFFNESS}"
}

run_broadcast() {
  echo "== Running broadcast =="
  echo "mpirun ${MPI_RUN_FLAGS} -np ${NP} ./bin/broadcast ${BCAST_ROUNDS} ${BCAST_ELEMENTS}"
  mpirun ${MPI_RUN_FLAGS} -np "${NP}" ./bin/broadcast "${BCAST_ROUNDS}" "${BCAST_ELEMENTS}"
}

case "${BIN}" in
  "")
    run_greetings
    echo
    run_ring
    echo
    run_broadcast
    ;;
  greetings)
    run_greetings
    ;;
  ring)
    run_ring
    ;;
  broadcast)
    run_broadcast
    ;;
  *)
    echo "Unknown binary: ${BIN}"
    echo "Supported values: greetings, ring, broadcast"
    exit 1
    ;;
esac
