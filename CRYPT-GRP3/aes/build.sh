#!/usr/bin/env bash
set -eu

SRCDIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SRCDIR"

g++ -O3 -std=c++11 -I./aes_interface \
    -c aes_interface/gmult.cpp -o gmult.o
g++ -O3 -std=c++11 -I./aes_interface \
    -c aes_interface/aes.cpp -o aes.o
g++ -O3 -std=c++11 -I./aes_interface \
    -c aes_interface/aes_interface.cpp -o aes_interface.o
g++ -O3 -std=c++11 -I./aes_interface \
    -c aes_interface/aes_main.cpp -o aes_main.o
g++ gmult.o aes.o aes_interface.o aes_main.o -o aes_test
./aes_test
