#!/bin/sh

export ROOT="$PWD/../.."

if [ ! -d mysh ]; then
    # Get source from GitHub
    git clone https://github.com/hungys/mysh.git

    # Apply patch
    cd mysh
    git apply ../mysh.patch
    cd ..
fi

# Build
cd mysh
make clean all
cd ..

# Copy into image
cp mysh/mysh $ROOT/sysroot/usr/bin
