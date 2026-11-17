#!/usr/bin/env bash
set -euo pipefail

g++ -std=c++17 -O3 -DNDEBUG -march=native -pthread \
  -Iparlaylib/include \
  baseline.cpp parallel_quicksort.cpp tests.cpp \
  -o /tmp/quicksort-benchmark

/tmp/quicksort-benchmark --benchmark \
  "${SIZE:-100000000}" \
  "${RUNS:-1}" \
  "${THREADS:-4}" \
  "${MIN_SPEEDUP:-0}"
