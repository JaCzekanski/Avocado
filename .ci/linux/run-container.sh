#!/bin/bash -ex
# Run in Travis to launch container

CLANG_VERSION=${1:-8}

mkdir -p "$HOME/.ccache"

docker run \
    -e CI=$CI \
    -v "$(pwd)":/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    avocadoemu/linux-clang$CLANG_VERSION \
    /bin/bash -ex /home/build/.ci/linux/build.sh