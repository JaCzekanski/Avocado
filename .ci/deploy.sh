#!/bin/bash -e

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <API_KEY> <FILE>"
    exit 1
fi

if [[ ! -z "${SKIP_DEPLOY}" ]]; then
    echo "Skipping deploy"
    exit 0
fi

URL=https://avocado-builds.czekanski.info/api/upload
FILES=""

if [ -f "$2" ]; then
    FILES="-F file=@\"$2\""
elif [ -d "$2" ]; then
    for entry in "$2"/*; do
        # skip dirs
        [ -f "$entry" ] || continue

        FILES="$FILES -F file=@\"$entry\""
    done
else
    echo "$2 does not exist"
    exit 2
fi

BRANCH=""
if [[ -n "${TRAVIS_BRANCH}" ]]; then
  BRANCH=$TRAVIS_BRANCH
elif [[ -n "${APPVEYOR_REPO_BRANCH}" ]]; then
  BRANCH=$APPVEYOR_REPO_BRANCH
elif [[ -n "${GITHUB_REF}" ]]; then
  BRANCH=${GITHUB_REF##*/}
else
  echo "TRAVIS_BRANCH, APPVEYOR_REPO_BRANCH or GITHUB_SHA env not found"
  exit 3
fi

REVISION="$(git rev-parse --short=7 HEAD)"
AUTHOR="$(git log -1 "$REVISION" --pretty="%cN")"
MESSAGE="$(git log -1 "$REVISION" --pretty="%s")"
DATE="$(git log -1 "$REVISION" --pretty="%cI")"

curl -X POST \
    -H "Authorization: Bearer $1" \
    -H "X-Commit-Revision: $REVISION" \
    -H "X-Commit-Branch: $BRANCH" \
    -H "X-Commit-Author: $AUTHOR" \
    -H "X-Commit-Message: $MESSAGE" \
    -H "X-Commit-Date: $DATE" \
    $FILES $URL \
    && echo Upload successful \
    || echo Upload failed
