#!/bin/sh

cd binutils-2.32
./build.sh
cd ..

cd gcc-8.3.0
./build.sh
cd ..