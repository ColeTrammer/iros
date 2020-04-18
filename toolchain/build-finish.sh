#!/bin/sh

set -e

cd gcc-9.2.0
./build-libstdc++-v3.sh
cd ..