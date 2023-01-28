#!/bin/sh

set -e

if [ ! "$ARCH" ] || [ ! "$SYSROOT" ];
then
    echo "ARCH, SYSROOT all must be set"
    exit 1
fi

GCC="$ARCH-iros-gcc"

LIB_PATH=`$GCC --print-file-name libgcc_s.so`
LIB_PATH=`dirname $LIB_PATH`

mkdir -p "$SYSROOT/usr/lib"
cp -pd $LIB_PATH/libstdc++.so* $LIB_PATH/libgcc_s.so* -t "$SYSROOT/usr/lib"
