#!/bin/bash
while inotifywait --recursive --quiet --event MODIFY src/
do
    make -s Avocado > /dev/null
    echo Done.
done
