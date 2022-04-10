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
    export SYSROOT="$ROOT/$OS_2_BUILD/os_2/sysroot"
    cd toolchain
    ./build.sh
    cd ..

    export PATH="$(./toolchain/path.sh):$PATH"
    export BUILD_TOOLCHAIN='1'
}

export OS_2_ARCH=${OS_2_ARCH:-'x86_64'}
export OS_2_BUILD="build_$OS_2_ARCH"
export OS_2_TARGET_OS='os_2'
export OS_2_TARGET="$OS_2_ARCH-$OS_2_TARGET_OS"
export OS_2_AS=${OS_AS:-"$OS_2_ARCH-$OS_2_TARGET_OS-as"}
export OS_2_CC=${OS_CC:-"$OS_2_ARCH-$OS_2_TARGET_OS-gcc"}
export OS_2_CXX=${OS_CXX:-"$OS_2_ARCH-$OS_2_TARGET_OS-g++"}
exists "$OS_2_CC" || maybe_build_toolchain

if exists 'ninja';
then
    DEFAULT_GENERATOR='Ninja'
else
    DEFAULT_GENERATOR='Unix Makefiles'
fi

GENERATOR="${GENERATOR:-$DEFAULT_GENERATOR}"

cmake -S cmake/super_build -B "$OS_2_BUILD" -G "$GENERATOR" || die "Failed to do create cmake project at ./build"

if [ "$BUILD_TOOLCHAIN" ];
then
    cd toolchain
    ./build-finish.sh
    cd ..
fi

echo "Finished the setup. Build files were created in ./$OS_2_BUILD"
if [ $BUILD_TOOLCHAIN ]; 
then
    echo "  To make sure the build tools are always available, please update your PATH as follows:"
    echo "    'PATH=$PATH'"
fi
