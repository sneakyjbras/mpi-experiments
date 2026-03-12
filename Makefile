MPICXX    ?= mpicxx
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
BIN_DIR   := bin

CPPFLAGS ?= -I$(INC_DIR)
CXXFLAGS ?= -O2 -std=c++23 -Wall -Wextra -Wpedantic
LDFLAGS  ?=

COMMON_SRCS := \
  $(SRC_DIR)/greetings.cpp \
  $(SRC_DIR)/mpi_utils.cpp \
  $(SRC_DIR)/ring.cpp \
  $(SRC_DIR)/broadcast.cpp

COMMON_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(COMMON_SRCS))

GREETINGS_MAIN := $(SRC_DIR)/greetings_main.cpp
RING_MAIN      := $(SRC_DIR)/ring_main.cpp
BROADCAST_MAIN := $(SRC_DIR)/broadcast_main.cpp

GREETINGS_OBJ := $(BUILD_DIR)/greetings_main.o
RING_OBJ      := $(BUILD_DIR)/ring_main.o
BROADCAST_OBJ := $(BUILD_DIR)/broadcast_main.o

TARGETS := $(BIN_DIR)/greetings $(BIN_DIR)/ring $(BIN_DIR)/broadcast
HEADERS := $(wildcard $(INC_DIR)/*.hpp)
LEGACY_FILES := \
  $(SRC_DIR)/main.cpp \
  $(SRC_DIR)/greetings_experiment.cpp \
  $(SRC_DIR)/ring_benchmark.cpp \
  $(SRC_DIR)/ring.hpp

.PHONY: all clean format run list legacy-check

all: legacy-check $(TARGETS)

list:
	@printf '%s\n' $(TARGETS)

legacy-check:
	@found=0; \
	for file in $(LEGACY_FILES); do \
	  if [ -e "$$file" ]; then \
	    if [ $$found -eq 0 ]; then \
	      echo "Ignoring legacy files left from the old layout:"; \
	      found=1; \
	    fi; \
	    echo "  - $$file"; \
	  fi; \
	done

$(BIN_DIR)/greetings: $(GREETINGS_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/ring: $(RING_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/broadcast: $(BROADCAST_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/greetings_main.o: $(GREETINGS_MAIN) $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ring_main.o: $(RING_MAIN) $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/broadcast_main.o: $(BROADCAST_MAIN) $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/greetings.o: $(SRC_DIR)/greetings.cpp $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/mpi_utils.o: $(SRC_DIR)/mpi_utils.cpp $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ring.o: $(SRC_DIR)/ring.cpp $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/broadcast.o: $(SRC_DIR)/broadcast.cpp $(HEADERS) | $(BUILD_DIR)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

format:
	find $(SRC_DIR) $(INC_DIR) -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +

run: all
	@NP=$${NP:-4}; ROUNDS=$${ROUNDS:-100000}; FEM_NODES=$${FEM_NODES:-256}; FEM_STIFFNESS=$${FEM_STIFFNESS:-4096}; BCAST_ROUNDS=$${BCAST_ROUNDS:-1000}; BCAST_ELEMENTS=$${BCAST_ELEMENTS:-1048576}; MPI_RUN_FLAGS=$${MPI_RUN_FLAGS:-}; BIN=$${BIN:-}; \
	if [ -n "$$BIN" ]; then \
	  case "$$BIN" in \
	    greetings) \
	      echo "== Running greetings =="; \
	      echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/greetings"; \
	      mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/greetings"; \
	      ;; \
	    ring) \
	      echo "== Running ring =="; \
	      echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/ring $$ROUNDS $$FEM_NODES $$FEM_STIFFNESS"; \
	      mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/ring" "$$ROUNDS" "$$FEM_NODES" "$$FEM_STIFFNESS"; \
	      ;; \
	    broadcast) \
	      echo "== Running broadcast =="; \
	      echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/broadcast $$BCAST_ROUNDS $$BCAST_ELEMENTS"; \
	      mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/broadcast" "$$BCAST_ROUNDS" "$$BCAST_ELEMENTS"; \
	      ;; \
	    *) \
	      echo "Unknown BIN=$$BIN. Supported values: greetings, ring, broadcast"; \
	      exit 1; \
	      ;; \
	  esac; \
	else \
	  echo "== Running greetings =="; \
	  echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/greetings"; \
	  mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/greetings"; \
	  echo; \
	  echo "== Running ring =="; \
	  echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/ring $$ROUNDS $$FEM_NODES $$FEM_STIFFNESS"; \
	  mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/ring" "$$ROUNDS" "$$FEM_NODES" "$$FEM_STIFFNESS"; \
	  echo; \
	  echo "== Running broadcast =="; \
	  echo "mpirun $$MPI_RUN_FLAGS -np $$NP $(BIN_DIR)/broadcast $$BCAST_ROUNDS $$BCAST_ELEMENTS"; \
	  mpirun $$MPI_RUN_FLAGS -np $$NP "$(BIN_DIR)/broadcast" "$$BCAST_ROUNDS" "$$BCAST_ELEMENTS"; \
	fi

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
