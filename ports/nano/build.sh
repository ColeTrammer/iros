#!/bin/sh

PORT_NAME=nano
SRC_DIR='nano-4.9.3'
BUILD_DIR='build-nano'
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install-strip'}

download() {
    # Download tar.gz
    curl https://www.nano-editor.org/dist/v4/nano-4.9.3.tar.gz --output nano-4.9.3.tar.gz

    # Extract contents
    tar -xzvf nano-4.9.3.tar.gz
}

patch() {
    git init
    git apply ../nano-4.9.3.patch
}

configure() {
    ../"$SRC_DIR"/configure --host=$HOST --prefix=/usr --disable-nls --disable-utf8
}

. ../.build_include.sh
