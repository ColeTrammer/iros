#!/bin/sh

set -e

die() {
    echo "$@" && exit 1
}

exists() {
    $1 --version >/dev/null 2>&1
}

export ARCH="$(uname -m)"
export TARGET_OS="$(uname)"

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

export TARGET_OS='os_2'
export ARCH="${OS_ARCH:-x86_64}"
export AS=${OS_AS:-"$ARCH-$TARGET_OS-as"}
export CC=${OS_CC:-"$ARCH-$TARGET_OS-gcc"}
export CXX=${OS_CXX:-"$ARCH-$TARGET_OS-g++"}

exists "$AS" || die "$AS does not exist" 
exists "$CC" || die "$CC does not exist" 
exists "$CXX" || die "$CXX does not exist" 

OS_2_BUILD_DIR="${OS_2_BUILD_DIR:-build}"
OS_2_BUILD_DIR="$(realpath $OS_2_BUILD_DIR)"
cmake -S . -B "$OS_2_BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_C_COMPILER_WORKS=TRUE \
    -DCMAKE_CXX_COMPILER_WORKS=TRUE \
    || die "Failed to create os cmake project at" "$OS_2_BUILD_DIR"

cmake --build "$OS_2_BUILD_DIR" --target install-libc

echo "Successfully created cmake build directories:"
echo "  'OS_2_NATIVE_DIR=$OS_2_NATIVE_DIR'"
echo "  'OS_2_BUILD_DIR=$OS_2_BUILD_DIR'"