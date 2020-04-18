#!/bin/sh

CC=gcc
CXX=g++
AS=as

die() {
    echo "$@" && exit 1
}

[ "$OS_2_BUILD_DIR" ] || die '$OS_2_BUILD_DIR must be set.' 

cmake --build "$OS_2_BUILD_DIR" --target install-libc

cd build-gcc

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3

cd ..