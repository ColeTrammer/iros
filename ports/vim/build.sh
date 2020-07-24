#!/bin/sh

PORT_NAME=vim
SRC_DIR='vim'
BUILD_DIR='vim'

download() {
    git clone https://github.com/vim/vim.git --depth=1
}

configure() {
    vim_cv_toupper_broken=0 vim_cv_terminfo=yes vim_cv_tgetent=zero vim_cv_getcwd_broken=no vim_cv_stat_ignores_slash=yes vim_cv_memmove_handles_overlap=yes ../"$SRC_DIR"/configure --host=$HOST --prefix=/usr --with-tlib=tinfo
}

. ../.build_include.sh
