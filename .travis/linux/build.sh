#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

cd /home/build

export OS=linux64
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT
export CC="ccache clang-6.0 -fcolor-diagnostics"
export CXX="ccache clang++-6.0 -fcolor-diagnostics"

# Configure cache
ccache --set-config=sloppiness=pch_defines,time_macros

# Download SDL2
apt update
apt install -y --no-install-recommends libsdl2-dev

# Generate Makefile
premake5 gmake

# Build
make config=release_x64 -j4
ccache -s

# Tests
./build/release_x64/avocado_test --use-colour yes

wget -nv https://gist.github.com/JaCzekanski/d7a6e06295729a3f81bd9bd488e9d37d/raw/d5bc41278fd198ef5e4afceb35e0587aca7f2f60/gte_valid_0xc0ffee_50.log
./build/release_x64/avocado_autotest gte_valid_0xc0ffee_50.log

# Package
mkdir -p $ARTIFACT
cp build/release_x64/avocado $ARTIFACT/avocado
cp -r data $ARTIFACT/

# Remove .gitignore
find $ARTIFACT -type f -name .gitignore -exec rm {} \;

mkdir -p upload
tar -zcf upload/$ARTIFACT.tar.gz $ARTIFACT
