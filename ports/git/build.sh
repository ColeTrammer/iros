#!/bin/sh

export PORT_NAME=git
export SRC_DIR='git'
export BUILD_DIR="$SRC_DIR"
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install'}

export ac_cv_fread_reads_directories=no
export ac_cv_snprintf_returns_bogus=no

export CFLAGS='-O2 -Wall -fno-omit-frame-pointer'
export HAVE_DEV_TTY=Yes
export NO_GETPAGESIZE=Yes
export NO_TCLTK=Yes

download() {
    git clone https://github.com/git/git --depth=1
}

patch() {
    git apply ../git.patch
}

configure() {
    export LDFLAGS="-lcurl"
    autoconf
    ./configure --host=$HOST --prefix=/usr --without-openssl --without-curl --with-editor=edit --with-pager=cat
}

build() {
    make -j5
}

. ../.build_include.sh
