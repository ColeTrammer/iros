#!/bin/sh

PORT_NAME=ncurses
SRC_DIR='ncurses-6.2'
BUILD_DIR='build-ncurses'

download() {
    # Download tar.gz
    curl ftp://ftp.invisible-island.net/ncurses/ncurses-6.2.tar.gz --output ncurses-6.2.tar.gz

    # Extract contents
    tar -xzvf ncurses-6.2.tar.gz
}

patch() {
    git init
    git apply ../ncurses-6.2.patch
}

configure() {
    ../"$SRC_DIR"/configure --host=$HOST --prefix=/usr --with-termlib --with-shared
}

. ../.build_include.sh
