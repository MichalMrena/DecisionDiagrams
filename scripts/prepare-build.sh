#!/bin/sh

# Choose a compiler
COMPILER=g++

# Remove old build files
rm -rf build/release
rm -rf build/debug

# Create build directories
mkdir -p build/release
mkdir -p build/debug

# Generate release Makefile
cd build/release
cmake -DCMAKE_CXX_COMPILER=$COMPILER \
      -DCMAKE_BUILD_TYPE=Release \
      -DLIBTEDDY_BUILD_TESTS=ON \
      -DLIBTEDDY_BUILD_EXAMPLES=ON \
      -DLIBTEDDY_BUILD_EXPERIMENTS=ON \
      ../..

# Generate debug Makefile
cd ../debug
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
      -DCMAKE_CXX_COMPILER=$COMPILER \
      -DCMAKE_BUILD_TYPE=Debug \
      -DLIBTEDDY_USE_SANITIZERS=ON \
      -DLIBTEDDY_BUILD_TESTS=ON \
      -DLIBTEDDY_BUILD_EXAMPLES=ON \
      -DLIBTEDDY_BUILD_EXPERIMENTS=ON \
      ../..

# Move compile commnads out of the build directory
mv compile_commands.json ../..