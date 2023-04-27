#!/bin/bash
rm -rf build/release
rm -rf build/debug
mkdir -p build/release
mkdir -p build/debug
cd build/release
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ../..
cd ../debug
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../..