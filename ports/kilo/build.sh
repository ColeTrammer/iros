#!/bin/sh

export ROOT="$PWD/../.."

# Get source from GitHub
git clone https://github.com/antirez/kilo.git

# Apply patch
cd kilo
git apply ../kilo.patch
cd ..

# Build
cd kilo
make all
cd ..

# Copy into image
cp kilo/kilo $ROOT/sysroot/usr/bin
