#!/usr/bin/env bash
set -euo pipefail

legacy_files=(
  src/main.cpp
  src/greetings_experiment.cpp
  src/ring_benchmark.cpp
  src/ring.hpp
)

found=0
for file in "${legacy_files[@]}"; do
  if [[ -e "$file" ]]; then
    if [[ $found -eq 0 ]]; then
      echo "Note: found legacy files from the old layout."
      echo "They are ignored by the current Makefile, but you can remove them:"
      found=1
    fi
    echo "  - $file"
  fi
done

make -j
