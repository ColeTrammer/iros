#!/bin/sh

set -e

die() {
    echo "$@" && exit 1
}

exists() {
    $1 --version >/dev/null 2>&1
}

build_toolchain() {
    export ROOT="$(realpath .)"
    export SYSROOT="$ROOT/$IROS_BUILD/iros/sysroot"
    export INSTALL_PREFIX="${IROS_TOOLCHAIN_PREFIX:-$ROOT/toolchain/cross}"
    cd toolchain
    ./build.sh
    cd ..

    export PATH="$INSTALL_PREFIX/bin:$PATH"
    export BUILD_TOOLCHAIN='1'
}

maybe_build_toolchain() {
    echo -n "Unable to find $IROS_ARCH-$IROS_TARGET_OS toolchain. Do you want to build it? (y/n) "
    read RESPONSE
    [ "$RESPONSE" = 'y' ] || return 1
    [ -d ./toolchain ] || die "Cannot find toolchain directory"

    build_toolchain
}

export IROS_ARCH=${IROS_ARCH:-'x86_64'}
export IROS_BUILD="build_$IROS_ARCH"
export IROS_TARGET_OS='iros'
export IROS_TARGET="$IROS_ARCH-$IROS_TARGET_OS"
export IROS_AS=${OS_AS:-"$IROS_ARCH-$IROS_TARGET_OS-as"}
export IROS_CC=${OS_CC:-"$IROS_ARCH-$IROS_TARGET_OS-gcc"}
export IROS_CXX=${OS_CXX:-"$IROS_ARCH-$IROS_TARGET_OS-g++"}

if [ "$FORCE_BUILD_TOOLCHAIN" ]; then
    build_toolchain
else
    exists "$IROS_CC" || maybe_build_toolchain
fi

if exists 'ninja';
then
    DEFAULT_GENERATOR='Ninja'
else
    DEFAULT_GENERATOR='Unix Makefiles'
fi

GENERATOR="${GENERATOR:-$DEFAULT_GENERATOR}"

cmake -S cmake/super_build -B "$IROS_BUILD" -G "$GENERATOR" || die "Failed to do create cmake project at ./build"

if [ "$BUILD_TOOLCHAIN" ];
then
    cd toolchain
    ./build-finish.sh
    cd ..
fi

echo "Finished the setup. Build files were created in ./$IROS_BUILD"
if [ $BUILD_TOOLCHAIN ]; 
then
    echo "  To make sure the build tools are always available, please update your PATH as follows:"
    echo "    'PATH=$PATH'"
fi
