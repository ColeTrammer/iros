#!/bin/sh

set -e

cd binutils-2.34
./build.sh
cd ..

cd gcc-9.2.0
./build.sh
cd ..