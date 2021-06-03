#!/bin/sh

set -e

mkdir -p cross

cd cmake
./build.sh
cd ..

cd binutils
./build.sh
cd ..

cd gcc
./build.sh
cd ..
