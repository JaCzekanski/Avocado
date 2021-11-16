#!/bin/bash -ex

export PREMAKE_VERSION="5.0.0-beta1"

# Download Premake5
wget -nv https://github.com/premake/premake-core/releases/download/v$PREMAKE_VERSION/premake-$PREMAKE_VERSION-macosx.tar.gz
tar xzf premake-$PREMAKE_VERSION-macosx.tar.gz
mv premake5 /usr/local/bin/
rm premake-$PREMAKE_VERSION-macosx.tar.gz

# Download SDL2 and ccache
brew update > /dev/null
brew install sdl2 ccache
