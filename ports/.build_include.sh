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

function_exists() {
    TYPE_RESULT="$(type $1 | head -n 1)"
    TYPE_TARGET="$1 is a shell function"
    [ "$TYPE_TARGET" = "$TYPE_RESULT" ]
}

builtin_exists() {
    TYPE_RESULT="$(type $1 | head -n 1)"
    TYPE_TARGET="$1 is a shell builtin"
    [ "$TYPE_TARGET" = "$TYPE_RESULT" ]
}

builtin_exists pushd || pushd() {
    export __OLD_PUSHD_DIRECTORY=$PWD
    cd $1
}

builtin_exists popd || popd() {
    cd $__OLD_PUSHD_DIRECTORY
}

MAKE_ARGS="$MAKE_ARGS -j5"

function_exists download || die "download() is not defined"

function_exists configure || configure() {
    :
}

function_exists clean || clean() {
    make clean $MAKE_ARGS
}

function_exists build || build() {
    make $MAKE_ARGS
}

function_exists patch || patch() {
    :
}

function_exists install || install() {
    make ${INSTALL_COMMAND:-install} DESTDIR="$ROOT/sysroot" $MAKE_ARGS
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
            pushd $BUILD_DIR
            clean || die "clean failed"
            popd
            ;;
        install)
            pushd $BUILD_DIR
            install || die "install failed"
            popd
            ;;
        build)
            pushd $BUILD_DIR
            build || die "build failed"
            popd
            ;;
        download)
            if [ ! -e $SRC_DIR ]; then
                download || die "download failed"
            fi
            ;;
        patch)
            pushd $SRC_DIR
            patch || die "patch failed"
            popd
            ;;
        configure)
            if [ ! -e $BUILD_DIR ]; then
                mkdir -p $BUILD_DIR
            fi
            pushd $BUILD_DIR
            configure || die "configure failed"
            popd
            ;;
        *)
            die "unrecognized target:" $TARGET
            ;;
    esac
}

run
