#!/usr/bin/env bash
set -euo pipefail

find src include -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
