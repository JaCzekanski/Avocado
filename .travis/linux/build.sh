#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

export CC="ccache clang-6.0 -fcolor-diagnostics"
export CXX="ccache clang++-6.0 -fcolor-diagnostics"

cd /home/build
export BASE=$PWD
export PATH=$PATH:$PWD/build/externals/SDL2/bin
export PREFIX=$PWD/build/externals/SDL2
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

# Check if SDL2 is available
if [ ! -d "$PREFIX" ]; then
    # Build SDL2
    mkdir -p build/externals/SDL2
    cd externals/SDL2
    ./configure --prefix=$PREFIX
    make -j4
    make install
else 
    echo "SDL2 already built, skipping"
fi

# Generate Makefile
cd $BASE
premake5 gmake

# Build
make config=release_x64 -j4
ccache -s

# Tests
./build/release_x64/avocado_test --use-colour yes --success || true

wget -nv https://gist.github.com/JaCzekanski/d7a6e06295729a3f81bd9bd488e9d37d/raw/d5bc41278fd198ef5e4afceb35e0587aca7f2f60/gte_valid_0xc0ffee_50.log
./build/release_x64/avocado_autotest gte_valid_0xc0ffee_50.log || true
