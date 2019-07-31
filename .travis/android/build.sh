#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

cd /home/build

export OS=android
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT.apk
export ASSETS_DIR=android/app/src/main/assets
export TARGET_DIR=android/app/build/outputs/apk/release
export NDK_CCACHE="$(which ccache)"

# Configure cache
ccache --set-config=sloppiness=pch_defines,time_macros

# Generate Makefile
premake5 --os=android androidmk

# Copy data to assets folder to embed it into .apk file
cp -r data $ASSETS_DIR
find $ASSETS_DIR -type f -name .gitignore -exec rm {} \;

# Build
pushd android

# Native code
NDK_PROJECT_PATH=`pwd`/app ndk-build PM5_CONFIG=release_x64 -j4 avocado NDK_DEBUG=0
ccache -s

# Java code
./gradlew assembleRelease
popd

# Tests
# No test suite for Android right now

# Package and prepare upload artifact
mkdir -p upload
cp $TARGET_DIR/app-release.apk upload/$ARTIFACT
