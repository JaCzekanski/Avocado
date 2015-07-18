#!/bin/sh
SRC=`find src/ | grep '\(\.cpp$\)'`
CXXFLAGS="-std=c++11 -Wno-write-strings `sdl2-config --cflags --libs`"
OUTPUT="build/avocado"
rm -rf build/
mkdir -p build
$CXX --version
$CXX $CXXFLAGS -o $OUTPUT $SRC
