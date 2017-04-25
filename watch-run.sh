#!/bin/bash
while inotifywait --quiet --recursive --event ATTRIB build/Debug/
do
    clear
    find /home/kuba/dev/psx-hardware-tests/build/tests/ -name "*.psexe" -exec ./build/Debug/Avocado {} \;
#    ./build/Debug/Avocado  /home/kuba/dev/psx-hardware-tests/build/tests/bios_write/bios_write.psexe
    echo Done.
done
