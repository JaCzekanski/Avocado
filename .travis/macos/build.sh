#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

export OS=macos
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT
export UPLOAD_DIR=$DATE-$COMMIT

# Configure cache
brew install ccache
export PATH="/usr/local/opt/ccache/libexec:$PATH"
ccache --set-config=sloppiness=pch_defines,time_macros

# Generate Makefile
premake5 xcode4

# Build
xcodebuild -workspace Avocado.xcworkspace -scheme avocado -configuration release -quiet

# Package
mkdir -p $ARTIFACT
cp -r build/release_x64/avocado.app $ARTIFACT/avocado.app
mkdir -p $ARTIFACT/avocado.app/Contents/Resources
cp -r data $ARTIFACT/avocado.app/Contents/Resources

# Remove .gitignore
find $ARTIFACT -type f -name .gitignore -exec rm {} \;

# Prepare upload artifact
mkdir -p upload/$UPLOAD_DIR
zip -r upload/$UPLOAD_DIR/$ARTIFACT.zip $ARTIFACT
