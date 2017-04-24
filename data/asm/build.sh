#!/bin/bash

FILE=bootstrap
BASE=0x1f000000

mipsel-unknown-elf-as -O0 -mtune=R2000 -mips1 -no-mips16 --fatal-warnings -o ${FILE}.o ${FILE}.s
mipsel-unknown-elf-ld -Ttext=${BASE} -o ${FILE}.elf ${FILE}.o
mipsel-unknown-elf-objcopy -O binary --only-section .text ${FILE}.elf ${FILE}.bin

rm ${FILE}.o
rm ${FILE}.elf
