#!/usr/bin/env bash
set -eu

SRCDIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SRCDIR"

g++ -O3 -I./sha256_optimized -I./sha256_optimized/sha256 \
    -c sha256_optimized/sha256/sha256.cpp -o sha256.o
g++ -O3 -I./sha256_optimized -I./sha256_optimized/sha256 \
    -c sha256_optimized/sha256/sha256_main.cpp -o sha256_main.o
g++ -O3 -I./sha256_optimized -I./sha256_optimized/sha256 \
    -c sha256_optimized/sha256/sha256_interface.cpp -o sha256_interface.o
g++ -O3 -I./sha256_optimized -I./sha256_optimized/sha256 \
    -c sha256_optimized/sha256/timestamp.cpp -o timestamp.o
g++ sha256.o sha256_main.o sha256_interface.o timestamp.o -o sha256_test_optimized
./sha256_test_optimized
