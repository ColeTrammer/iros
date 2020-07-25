#!/bin/sh

PORT_NAME=less
SRC_DIR='less-530'
BUILD_DIR='build-less'
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install-strip'}

download() {
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/less/less-530.tar.gz --output less-530.tar.gz

    # Extract contents
    tar -xzvf less-530.tar.gz
}

configure() {
    "../$SRC_DIR/configure" --host=$HOST --prefix=/usr --with-editor=edit
}

. ../.build_include.sh
