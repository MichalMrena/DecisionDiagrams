#!/bin/bash
mkdir -p build/release
mkdir -p build/debug
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cd ../debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..