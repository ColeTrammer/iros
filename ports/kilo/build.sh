#!/bin/sh

export ROOT="$PWD/../.."


if [ ! -d kilo ]; then
    # Get source from GitHub
    git clone https://github.com/antirez/kilo.git

    # Apply patch
    cd kilo
    git apply ../kilo.patch
    cd ..
fi

# Build
cd kilo
make all
cd ..

# Copy into image
cp kilo/kilo $ROOT/sysroot/usr/bin
