#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

cd /home/build

export BUILD_MODE=release
if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then 
    export BUILD_MODE=debug
fi

export OS=android
export DATE=$(date +%Y%m%d)
export COMMIT=$(git rev-parse --short=7 HEAD)
export ARTIFACT=avocado-$OS-$DATE-$COMMIT.apk
export ASSETS_DIR=android/app/src/main/assets
export TARGET_DIR=android/app/build/outputs/apk/$BUILD_MODE
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
./gradlew assemble$BUILD_MODE
popd

# Tests
# No test suite for Android right now

# Package and prepare upload artifact
mkdir -p upload
cp $TARGET_DIR/app-$BUILD_MODE.apk upload/$ARTIFACT
