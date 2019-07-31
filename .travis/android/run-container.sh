#!/bin/bash -ex
# Run in Travis to launch container

mkdir -p "$HOME/.ccache"

docker run \
    -v $(pwd):/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    -e keystore_password="$keystore_password" \
    avocadoemu/android \
    /bin/bash -ex /home/build/.travis/android/build.sh
