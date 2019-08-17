#!/bin/bash -ex
# Run in Travis to launch container

CLANG_VERSION=${1:-6}

mkdir -p "$HOME/.ccache"

docker run \
    -v $(pwd):/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    avocadoemu/linux-clang$CLANG_VERSION \
    /bin/bash -ex /home/build/.travis/linux/build.sh

