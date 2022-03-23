#!/bin/bash -ex
# Run by run-container.sh, builds Avocado in Docker container and runs tests

export OS=macos
export DATE=`date +%Y%m%d`
export COMMIT=`git rev-parse --short=7 HEAD`
export ARTIFACT=avocado-$OS-$DATE-$COMMIT

# Configure cache
brew install ccache
export PATH="/usr/local/opt/ccache/libexec:$PATH"
ccache --set-config=sloppiness=pch_defines,time_macros

# Generate Makefile
premake5 xcode4

# Build
set -o pipefail && xcodebuild -workspace Avocado.xcworkspace -scheme avocado -configuration release -parallelizeTargets -jobs 4 | xcpretty

# Package
mkdir -p $ARTIFACT
cp -r build/release_x64/avocado.app $ARTIFACT/avocado.app
mkdir -p $ARTIFACT/avocado.app/Contents/Resources
cp -r data $ARTIFACT/avocado.app/Contents/Resources
cp misc/macos/Info.plist $ARTIFACT/avocado.app/Contents

# Copy icon
mv misc/avocado.icns $ARTIFACT/avocado.app/Contents/Resources/

# Copy & fix dylibs
dylibbundler -od -b \
  -x $ARTIFACT/avocado.app/Contents/MacOS/avocado \
  -d $ARTIFACT/avocado.app/Contents/Frameworks/ \
  -p @executable_path/../Frameworks/

# Remove .gitignore
find $ARTIFACT -type f -name .gitignore -exec rm {} \;

# Prepare upload artifact
mkdir -p upload
zip -r upload/$ARTIFACT.zip $ARTIFACT
