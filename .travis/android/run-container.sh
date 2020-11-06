#!/bin/bash -ex
# Run in Travis to launch container

# Setup signing
export BUILD_MODE="debug"
if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then 
    openssl aes-256-cbc -K $encrypted_7333c7dd5b15_key -iv $encrypted_7333c7dd5b15_iv -in android/avocado.keystore.enc -out android/avocado.keystore -d
    export BUILD_MODE="release"
fi

if [ "$GITHUB_ACTIONS" = "true" ]; then
    export BUILD_MODE="debug"
fi

mkdir -p "$HOME/.ccache"

docker run \
    -e CI=$CI \
    -e BUILD_MODE=$BUILD_MODE \
    -e keystore_password="$keystore_password" \
    -v $(pwd):/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    avocadoemu/android \
    /bin/bash -ex /home/build/.travis/android/build.sh
