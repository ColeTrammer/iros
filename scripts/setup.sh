#!/bin/sh

set -e

die() {
    echo "$@" && exit 1
}

exists() {
    $1 --version >/dev/null 2>&1
}

maybe_build_toolchain() {
    echo -n "Unable to find os_2 toolchain. Do you want to build it? (y/n) "
    read RESPONSE
    [ "$RESPONSE" = 'y' ] || return 1
    [ -d ./toolchain ] || die "Cannot find toolchain directory"
    
    export ROOT="$(realpath .)"
    cd toolchain
    ./build.sh
    cd ..

    export PATH="$(./toolchain/path.sh):$PATH"
    export BUILD_TOOLCHAIN='1'
}

if exists 'ninja';
then
    DEFAULT_GENERATOR='Ninja'
else
    DEFAULT_GENERATOR='Unix Makefiles'
fi

GENERATOR="${GENERATOR:-$DEFAULT_GENERATOR}"

export OS_2_NATIVE_DIR="${OS_2_NATIVE_DIR:-native}"
export OS_2_NATIVE_DIR="$(realpath $OS_2_NATIVE_DIR)"

cmake -S . -B "$OS_2_NATIVE_DIR" -G "$GENERATOR" || die "Failed to do create native cmake project at" "$OS_2_NATIVE_DIR"

cmake --build "$OS_2_NATIVE_DIR" || die "Failed to do native build"

export ARCH='x86_64'
export TARGET_OS='os_2'
export AS=${OS_AS:-"$ARCH-$TARGET_OS-as"}
export CC=${OS_CC:-"$ARCH-$TARGET_OS-gcc"}
export CXX=${OS_CXX:-"$ARCH-$TARGET_OS-g++"}

echo "$(realpath .)"
exists "$AS" || maybe_build_toolchain || die "$AS does not exist" 
exists "$CC" || die "$CC does not exist" 
exists "$CXX" || die "$CXX does not exist" 

cmake -S . -B "$OS_2_BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_C_COMPILER_WORKS=TRUE \
    -DCMAKE_CXX_COMPILER_WORKS=TRUE \
    -DCMAKE_TOOLCHAIN_FILE=cmake/CMakeToolchain.txt \
    || die "Failed to create os cmake project at" "$OS_2_BUILD_DIR"

if [ "$BUILD_TOOLCHAIN" ];
then
    cd toolchain
    ./build-finish.sh
    cd ..
fi

echo "Successfully created cmake build directories:"
echo "  'OS_2_NATIVE_DIR=$OS_2_NATIVE_DIR'"
echo "  'OS_2_BUILD_DIR=$OS_2_BUILD_DIR'"
if [ $BUILD_TOOLCHAIN ]; 
then
    echo "  'PATH=$PATH'"
fi
