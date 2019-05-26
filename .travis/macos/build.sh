#!/bin/bash -e
# Run by run-container.sh, builds Avocado in Docker container and runs tests

export OS=macos
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT.app
export UPLOAD_DIR=$DATE-$COMMIT

# Generate Makefile
premake5 xcode4

# Build
xcodebuild -workspace Avocado.xcworkspace -scheme avocado -configuration release

# Package
mkdir -p artifact
cp -r build/release_x64/avocado.app artifact/avocado.app
cp -r data artifact/avocado.app/Contents/MacOS/

# Remove .gitignore and asm dir
find artifact -type f -name .gitignore -exec rm {} \;
rm -r artifact/avocado.app/Contents/MacOS/data/asm

# Prepare upload artifact
mkdir -p upload/$UPLOAD_DIR
tar -zcf upload/$UPLOAD_DIR/$ARTIFACT.tar.gz artifact/avocado.app
