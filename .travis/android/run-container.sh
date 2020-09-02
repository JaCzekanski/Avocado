#!/bin/bash -ex
# Run in Travis to launch container

# Setup signing
if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then 
    openssl aes-256-cbc -K $encrypted_7333c7dd5b15_key -iv $encrypted_7333c7dd5b15_iv -in android/avocado.keystore.enc -out android/avocado.keystore -d
fi

mkdir -p "$HOME/.ccache"

docker run \
    -e CI=$CI \
    -e TRAVIS_PULL_REQUEST=$TRAVIS_PULL_REQUEST \
    -v $(pwd):/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    -e keystore_password="$keystore_password" \
    avocadoemu/android \
    /bin/bash -ex /home/build/.travis/android/build.sh
