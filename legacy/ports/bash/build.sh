#!/bin/sh

export PORT_NAME=bash
export SRC_DIR='bash-5.0'
export BUILD_DIR='build-bash'
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install-strip'}

download() {
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/bash/bash-5.0.tar.gz --output bash-5.0.tar.gz

    # Extract contents
    tar -xzvf bash-5.0.tar.gz
}

patch() {
    git init
    git apply ../bash-5.0.patch
    git apply ../bash-5.0-loadables.patch
}

configure() {
    export bash_cv_getcwd_malloc=yes
    ../bash-5.0/configure --host=$HOST --disable-nls --without-bash-malloc --prefix=/usr
}

. ../.build_include.sh
