# mpi-experiments

Small MPI experiments in modern C++ for learning the main communication patterns in distributed-memory programs.

The project is intentionally simple and explicit:
- one experiment for basic point-to-point messaging
- one experiment for ring communication with multiple payload types
- one experiment for broadcast, comparing the usual collective against a manual implementation

That structure is deliberate. It lets us study MPI progressively:
1. first, verify that ranks can talk to each other at all
2. then measure neighbor-to-neighbor communication cost
3. then compare a manual one-to-many pattern against the optimized collective that MPI already provides

---

## Experiments implemented so far

### 1. `greetings`
A minimal `MPI_Send` / `MPI_Recv` sanity-check.

Implemented in:
- `src/greetings.cpp`
- `src/greetings_main.cpp`
- `include/mpi_greetings.hpp`

What it does:
- each non-root rank sends a greeting to rank `0`
- the message includes the task id and processor name
- rank `0` receives the message and sends a reply back

Why it exists:
- validates MPI initialization/finalization
- validates rank discovery and processor-name discovery
- validates blocking point-to-point communication
- gives a very small, easy-to-debug first MPI program

Why the output looks the way it does:
- in your run, all tasks printed `lab2p2` as the machine name because the 4 MPI ranks were launched on the same node
- if you run across multiple machines, different ranks may print different node names

Example run:

```text
== Running greetings ==
mpirun  -np 4 ./bin/greetings
Task 0 is receiving greetings on machine lab2p2
Task 0 received: Task 1 sends greetings from machine lab2p2
Task 0 received: Task 2 sends greetings from machine lab2p2
Task 1 received reply: Task 0 sends greetings back from machine lab2p2 to task 1
Task 2 received reply: Task 0 sends greetings back from machine lab2p2 to task 2
Task 0 received: Task 3 sends greetings from machine lab2p2
Task 3 received reply: Task 0 sends greetings back from machine lab2p2 to task 3
```

---

### 2. `ring`
A ring-based MPI micro-benchmark for point-to-point communication.

Implemented in:
- `src/ring.cpp`
- `src/ring_main.cpp`
- `include/mpi_ring.hpp`

Payloads currently benchmarked:
- `int32 token`
- `double scalar`
- `small POD struct`
- `FEM heat patch`

#### What algorithm is in `ring.cpp`?
It is a **ring token-passing send/recv benchmark**.

How it works:
- the processes are arranged logically in a ring
- rank `0` sends a payload to rank `1`
- every intermediate rank receives from its left neighbor and forwards to its right neighbor
- the last rank sends the payload back to rank `0`
- this is repeated for many rounds

For `p` processes:
- rank `0` sends to `1` and receives from `p - 1`
- rank `r > 0` receives from `r - 1`
- rank `r > 0` sends to `(r + 1) mod p`

#### Why this experiment is built this way
This is not a load-balancing or failure-detection algorithm.
It is a communication micro-benchmark.

Its purpose is to study:
- nearest-neighbor point-to-point messaging
- message startup cost for small payloads
- effective throughput for larger payloads
- how the same communication pattern reacts to different data layouts

It is useful because ring-like communication appears naturally in:
- token passing
- distributed pipelines
- halo exchange intuition
- staged coordination between ranks

#### Why several payload types were added
Using only one integer would tell us very little. The current benchmark intentionally spans:
- a tiny primitive (`int`)
- another tiny primitive (`double`)
- a small trivially-copyable struct
- a larger C++ scientific-style object

That lets us see the classic transition:
- **small messages** are mostly latency-dominated
- **large messages** amortize startup costs and reveal bandwidth more clearly

#### The large scientific object
The large payload is a playful FEM-like object called `HeatDiffusionPatch`.

It contains:
- scalar `double` metadata
- a nested `CellMaterial` struct
- `std::vector<double>` fields for nodal temperatures and a local stiffness matrix

Why this object exists:
- real scientific codes rarely send just integers
- they often send structured state with metadata plus numeric arrays
- this gives a more realistic example than a single scalar token

Important note:
- the large object is serialized into a contiguous byte buffer before sending
- this is necessary because classes containing `std::vector` are not safe to transmit as raw memory blobs with a trivial MPI datatype

Example run:

```text
== Running ring ==
mpirun  -np 4 ./bin/ring 100000 256 4096
Ring MPI micro-benchmarks
Processes = 4, Rounds = 100000
FEM object = HeatDiffusionPatch with 256 nodes and 4096 stiffness entries

Payload                    Bytes      Time (s)    Latency (us/hop)     Bandwidth (MiB/s)
----------------------------------------------------------------------------------------
int32 token                    4      0.084328               0.211                18.095
double scalar                  8      0.083767               0.209                36.431
small POD struct              40      0.087746               0.219               173.897
FEM heat patch             34880      2.295603               5.739              5796.152

Definitions:
- Latency is the average time for one message hop around the ring.
- Bandwidth is the effective payload throughput: payload_bytes * rounds * processes / total_time.
- The large FEM object is serialized first because std::vector makes the class non-trivial to send directly with raw MPI datatypes.
```

#### Interpreting the ring results
A few things are visible immediately:
- the `int` and `double` cases have almost identical latency, because startup/synchronization dominates tiny messages
- the 40-byte POD struct is still mostly in the small-message regime
- the FEM object has much higher effective bandwidth because the payload is large enough to amortize startup overhead
- its latency per hop is higher, but the throughput is far better because much more data is moved per message

---

### 3. `broadcast`
A large-array broadcast benchmark comparing two implementations.

Implemented in:
- `src/broadcast.cpp`
- `src/broadcast_main.cpp`
- `include/mpi_broadcast.hpp`

Methods compared:
- `MPI_Bcast`
- manual root-to-all `MPI_Send` / `MPI_Recv`

What it does:
- allocates a large `std::vector<double>`
- repeats many broadcast rounds
- times the usual MPI collective
- times a naive manual broadcaster where rank `0` sends the whole array to each other rank explicitly

Why this experiment is built this way:
- it compares the **idiomatic MPI solution** against the **obvious manual solution**
- it shows why collectives usually exist: they express the communication pattern clearly and can be optimized by the MPI library
- it gives a concrete baseline for one-to-many communication

Example run:

```text
== Running broadcast ==
mpirun  -np 4 ./bin/broadcast 1000 1048576
Broadcast MPI micro-benchmarks
Processes = 4, Rounds = 1000
Array size = 1048576 doubles (8388608 bytes)

Method                     Bytes      Time (s)    Latency (us/bcast)     Bandwidth (MiB/s)
------------------------------------------------------------------------------------------
MPI_Bcast array          8388608      2.750638              2750.638              8725.248
Manual Send/Recv         8388608      2.957440              2957.440              8115.127

Definitions:
- Latency is the average time per broadcast operation.
- Bandwidth is the effective application-level throughput:
  payload_bytes * rounds * (processes - 1) / total_time.
- MPI_Bcast is the usual optimized collective implementation, while
  Manual Send/Recv uses rank 0 as a simple explicit broadcaster.
```

#### Interpreting the broadcast results
In your run, `MPI_Bcast` beat the manual version both in latency and bandwidth.
That is exactly what we would usually expect.

Why:
- the manual version makes rank `0` perform all sends itself
- this creates a root bottleneck
- `MPI_Bcast` can use a better internal algorithm than simple root-to-all serial sends
- collective implementations are often tree-based, pipelined, topology-aware, or otherwise optimized by the MPI library

So the benchmark is useful because it demonstrates an important MPI lesson:
> when the communication pattern is a broadcast, the collective is usually the right first choice

---

## Measured quantities used by this project

### Ring benchmark metrics
The code prints:

```text
latency_us_per_hop = total_seconds * 1e6 / (rounds * processes)
bandwidth_mib_s    = payload_bytes * rounds * processes / total_seconds / (1024 * 1024)
```

Interpretation:
- `latency_us_per_hop` is the average time for one send/recv hop in the ring
- `bandwidth_mib_s` is the effective payload throughput moved by the ring benchmark

### Broadcast benchmark metrics
The code prints:

```text
latency_us_per_bcast = total_seconds * 1e6 / rounds
bandwidth_mib_s      = payload_bytes * rounds * (processes - 1) / total_seconds / (1024 * 1024)
```

Interpretation:
- `latency_us_per_bcast` is the average cost of one complete broadcast
- `bandwidth_mib_s` is the effective application-level rate from the root to all non-root processes

---

## Communication-cost formulas (α–β model)

A useful simple performance model for MPI is:

```text
T(m) ≈ α + βm
```

Where:
- `α` = latency / startup cost per message
- `β` = transfer time per byte (or per data unit)
- `m` = message size

This is the standard first-order way to reason about communication cost.

### Point-to-point `MPI_Send` / `MPI_Recv`
For one blocking point-to-point transfer of message size `m`:

```text
T_sendrecv(m) ≈ α + βm
```

So asymptotically:
- startup term: `O(1)` messages
- payload term: `O(m)` data volume

### Ring communication with `p` processes
One full circulation of the payload around the ring uses `p` hops, so:

```text
T_ring_one_round(m, p) ≈ p(α + βm)
```

For `R` rounds:

```text
T_ring_total(m, p, R) ≈ R · p(α + βm)
```

This is why small messages mainly expose `α`, while large messages increasingly expose `βm`.

### Manual broadcast with root `MPI_Send` / `MPI_Recv`
In the naive root-to-all version, rank `0` sends the full message separately to every other rank:

```text
T_manual_bcast(m, p) ≈ (p - 1)(α + βm)
```

So the root does linear work in the number of processes.
This is the main scalability weakness of the manual implementation.

### Optimized `MPI_Bcast`
`MPI_Bcast` is a collective, and its exact cost depends on the algorithm selected internally by the MPI implementation.
A common simplified model for a tree-based broadcast is:

```text
T_bcast_tree(m, p) ≈ log2(p)(α + βm)
```

or equivalently, up to constants,

```text
T_bcast_tree(m, p) = O(log p · (α + βm))
```

This is why optimized broadcast is usually better than manual root-to-all sending:
- fewer serialized startup steps than `O(p)` root sends
- better distribution of communication work
- less bottleneck pressure on the root process

Important note:
- real MPI libraries may use different broadcast algorithms depending on message size, process count, network, and topology
- so `MPI_Bcast` should be thought of as **implementation-dependent but usually much better optimized** than a naive manual broadcaster

---

## How to build and run on Manjaro

### 1. Install Open MPI
On Manjaro, install an MPI implementation such as Open MPI.
A typical setup is:

```bash
sudo pacman -S openmpi
```

Then verify that the wrapper compiler and launcher are available:

```bash
mpicxx --version
mpirun --version
```

### 2. Build the project
From the repository root:

```bash
make
```

or:

```bash
./build.sh
```

This builds:
- `bin/greetings`
- `bin/ring`
- `bin/broadcast`

### 3. Run all experiments

```bash
./run.sh
```

By default the script runs:
1. `greetings`
2. `ring`
3. `broadcast`

### 4. Run each experiment individually

```bash
./run.sh greetings
./run.sh ring
./run.sh broadcast
```

### 5. Override parameters
You can change the default run parameters through environment variables.

#### Ring

```bash
NP=4 ROUNDS=100000 FEM_NODES=256 FEM_STIFFNESS=4096 ./run.sh ring
```

#### Broadcast

```bash
NP=4 BCAST_ROUNDS=1000 BCAST_ELEMENTS=1048576 ./run.sh broadcast
```

### 6. Direct `mpirun` examples

```bash
mpirun -np 4 ./bin/greetings
mpirun -np 4 ./bin/ring 100000 256 4096
mpirun -np 4 ./bin/broadcast 1000 1048576
```

---

## Why this project is organized this way

The repo is deliberately split into:
- `include/` for declarations
- `src/` for implementations
- a small `Makefile` and helper scripts for repeatable runs

That keeps the code:
- easy to read
- easy to extend with new MPI experiments
- easy to benchmark without changing the source every time

This is a good structure for a learning repository, because each experiment stays small and focused while still sharing the same build and run flow.

---

## Project structure

```text
mpi-experiments/
├── Makefile
├── README.md
├── build.sh
├── format.sh
├── run.sh
├── docs/
│   ├── aulaMPI.pdf
│   └── lab-mpi-guide.pdf
├── include/
│   ├── mpi_broadcast.hpp
│   ├── mpi_greetings.hpp
│   ├── mpi_ring.hpp
│   └── mpi_utils.hpp
└── src/
    ├── broadcast.cpp
    ├── broadcast_main.cpp
    ├── greetings.cpp
    ├── greetings_main.cpp
    ├── mpi_utils.cpp
    ├── ring.cpp
    └── ring_main.cpp
```

---

## Summary

So far, the project covers three very useful MPI ideas:
- **basic point-to-point messaging** with `greetings`
- **neighbor communication and message-size effects** with `ring`
- **collective broadcast vs naive manual broadcast** with `broadcast`

That progression is pedagogically useful because it moves from:
- correctness and visibility
- to micro-benchmarking
- to comparing an explicit pattern against the MPI collective designed for it
