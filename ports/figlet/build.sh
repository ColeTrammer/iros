#!/bin/sh

export ROOT="$PWD/../.."


if [ ! -d figlet-2.2.5 ]; then
    # Download tar.gz
    curl ftp://ftp.figlet.org/pub/figlet/program/unix/figlet-2.2.5.tar.gz --output figlet.tar.gz

    # Extract contents
    tar -xzvf figlet.tar.gz

    # Apply patch
    cd figlet-2.2.5
    git init
    git apply ../figlet-2.2.5.patch
    cd ..
fi

# Build and install
cd figlet-2.2.5
make clean install
cd ..

# Copy into image
cp -r figlet/bin $ROOT/sysroot/usr
cp -r figlet/share $ROOT/sysroot