#!/bin/sh

# Choose a compiler
C_COMPILER=gcc
CXX_COMPILER=g++
BUILD_TYPE=Debug

# Remove old build files
rm -rf build

# Create build directories
mkdir -p build

# Generate debug Makefile
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_C_COMPILER=$C_COMPILER     \
      -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE     \
      -DLIBTEDDY_USE_SANITIZERS=ON       \
      -DLIBTEDDY_BUILD_TESTS=ON          \
      -DLIBTEDDY_BUILD_EXAMPLES=ON       \
      -DLIBTEDDY_SYMBOLIC_RELIABILITY=ON \
      -DLIBTEDDY_ARBITRARY_PRECISION=ON  \
      -DLIBTEDDY_VERBOSE=OFF             \
      -DLIBTEDDY_COLLECT_STATS=OFF       \
      -DLIBTEDDY_USE_LIBCXX=OFF          \
      -DLIBTEDDY_USE_MOLD=OFF            \
      ..
