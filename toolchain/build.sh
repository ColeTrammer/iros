#!/bin/sh

set -e

cd binutils-2.31.1
./build.sh
cd ..

cd gcc-8.3.0
./build.sh
cd ..