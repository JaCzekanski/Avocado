#!/bin/sh
rm -rf build/
mkdir -p build
g++ src/main.cpp src/mipsInstructions.cpp src/utils/file.cpp src/utils/string.cpp `sdl2-config --cflags --libs` -std=c++11 -Wno-write-strings -o build/avocado
