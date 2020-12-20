#!/bin/bash -ex
# Run in Travis to launch container

# Setup signing
export BUILD_MODE="debug"
if [[ -n "$GITHUB_HEAD_REF" ]]; then
    gpg --quiet --yes --decrypt --passphrase="$KEYSTORE_KEY" --output android/avocado.keystore android/avocado.keystore.gpg
    export BUILD_MODE="release"
fi

mkdir -p "$HOME/.ccache"

docker run \
    -e CI="$CI" \
    -e BUILD_MODE="$BUILD_MODE" \
    -e KEYSTORE_PASSWORD="$KEYSTORE_PASSWORD" \
    -v "$(pwd)":/home/build \
    -v "$HOME/.ccache":/root/.ccache \
    avocadoemu/android \
    /bin/bash -ex /home/build/.ci/android/build.sh
