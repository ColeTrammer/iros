#!/bin/sh

PORT_NAME=make
SRC_DIR='make-4.3'
BUILD_DIR='build-make'
INSTALL_COMMAND='install-strip'

download() {
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/make/make-4.3.tar.gz --output make-4.3.tar.gz

    # Extract contents
    tar -xzvf make-4.3.tar.gz
}

patch() {
    git init
    git apply ../make-4.3.patch
}

configure() {
    ../make-4.3/configure --host=$HOST --disable-nls --prefix=/usr --without-guile
}

. ../.build_include.sh