#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

function buildSDL2 {
    pushd externals/SDL2

    ./configure
    make -j4
    make install

    popd
}

cd /home/build

export OS=linux64
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export VERSION=$COMMIT
export ARTIFACT=avocado-$OS-$DATE-$COMMIT
export CC="ccache cc -fcolor-diagnostics"
export CXX="ccache c++ -fcolor-diagnostics"

# Configure cache
ccache --set-config=sloppiness=pch_defines,time_macros

buildSDL2

# Generate Makefile
premake5 gmake

# Build
make config=release_x64 -j4
ccache -s

# Tests
./build/release_x64/avocado_test --use-colour yes

wget -ncv https://gist.github.com/JaCzekanski/d7a6e06295729a3f81bd9bd488e9d37d/raw/d5bc41278fd198ef5e4afceb35e0587aca7f2f60/gte_valid_0xc0ffee_50.log
./build/release_x64/avocado_autotest gte_valid_0xc0ffee_50.log

# Package
mkdir -p $ARTIFACT
mkdir -p $ARTIFACT/usr/share/avocado
cp -r data $ARTIFACT/usr/share/avocado/
cp android/app/src/main/ic_launcher-web.png $ARTIFACT/avocado.png

# Remove .gitignore
find $ARTIFACT -type f -name .gitignore -exec rm {} \;

wget -ncv https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x ./linuxdeploy-x86_64.AppImage

APPIMAGE_EXTRACT_AND_RUN=1 ./linuxdeploy-x86_64.AppImage \
    --appdir=$ARTIFACT \
    --executable=./build/release_x64/avocado \
    --create-desktop-file \
    --icon-file=$ARTIFACT/avocado.png \
    --output=appimage

# Upload
mkdir -p upload
mv avocado-*.AppImage upload/