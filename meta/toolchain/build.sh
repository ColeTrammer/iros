#!/bin/sh

set -e

PARENT_DIR=$(realpath $(dirname -- "$0"))

mkdir -p /tmp/binutils
cd /tmp/binutils
"$PARENT_DIR"/binutils/build.sh
rm -rf /tmp/binutils

mkdir -p /tmp/gcc
cd /tmp/gcc
"$PARENT_DIR"/gcc/build.sh
rm -rf /tmp/gcc
