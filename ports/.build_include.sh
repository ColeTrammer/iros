set -e

export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

die() {
    echo "Error:" "$@" && exit 1
}

list_target() {
    echo "List of targets:"
    echo "  download: download and unzip files"
    echo "  patch: change the files to work"
    echo "  configure: prepare for build"
    echo "  build: compile the port"
    echo "  install: install port to sysroot"
    echo "  all (default): all of the above"
    echo "  clean: delete build files"
}

if [ ! $PORT_NAME ] || [ ! $SRC_DIR ] || [ ! $BUILD_DIR ]; then
    die "incorrect invocation of .build_include.sh"
fi

TARGET=${1:-all}
if [ $TARGET = '--help' ]; then
    echo "Usage:" "./build.sh" "[target]"
    list_targets
    exit 0
fi

fake_pushd() {
    export __OLD_PUSHD_DIRECTORY=$PWD
    cd $1
}

fake_popd() {
    cd $__OLD_PUSHD_DIRECTORY
}

run() {
    case $TARGET in
        all)
            TARGET=download run 
            TARGET=patch run 
            TARGET=configure run 
            TARGET=build run 
            TARGET=install run 
            ;;
        clean)
            fake_pushd $BUILD_DIR
            clean || die "clean failed"
            fake_popd
            ;;
        install)
            fake_pushd $BUILD_DIR
            install || die "install failed"
            fake_popd
            ;;
        build)
            fake_pushd $BUILD_DIR
            build || die "build failed"
            fake_popd
            ;;
        download)
            if [ ! -e $SRC_DIR ]; then
                download || die "download failed"
            fi
            ;;
        patch)
            fake_pushd $SRC_DIR
            patch || die "patch failed"
            fake_popd
            ;;
        configure)
            if [ ! -e $BUILD_DIR ]; then
                fake_pushd ../..
                make install-headers
                rm sysroot/usr/include/dlfcn.h
                fake_popd

                mkdir -p $BUILD_DIR
                fake_pushd $BUILD_DIR
                configure || die "configure failed"
                fake_popd
            fi
            ;;
        *)
            die "unrecognized target:" $TARGET
            ;;
    esac
}

run