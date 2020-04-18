#!/bin/sh

ROOT="${ROOT:-$PWD/..}"

echo $(realpath "$ROOT/toolchain/cross/bin")