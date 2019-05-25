#!/bin/bash -ex

export OS=macos
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT

export CC="ccache clang"
export CXX="ccache clang++"

# Configure cache
ccache --set-config=sloppiness=pch_defines,time_macros

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
export TARGETDIR=build/release_x64
cp $TARGETDIR/avocado.app $ARTIFACT/avocado.app
cp -r data $ARTIFACT/

# Remove .gitignore and asm dir
find $ARTIFACT -type f -name .gitignore -exec rm {} \;
rm -r $ARTIFACT/data/asm

export UPLOAD_DIR=$DATE-$COMMIT
mkdir -p upload/$UPLOAD_DIR
tar -zcf upload/$UPLOAD_DIR/$ARTIFACT.tar.gz $ARTIFACT
