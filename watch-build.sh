#!/bin/bash
while inotifywait --recursive --quiet --event MODIFY src/
do
    clear
    make -s Avocado > /dev/null
    echo Done.
done
