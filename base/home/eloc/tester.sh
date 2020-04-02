#!/bin/sh

PROGRAMS="$(ls /initrd)"

for PROGRAM in $PROGRAMS; do
    echo "running: " "$PROGRAM"
    "$PROGRAM"
done