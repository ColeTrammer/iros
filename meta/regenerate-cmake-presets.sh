#!/bin/sh

set -e

PARENT_DIR=$(realpath $(dirname -- "$0"))
IROS_ROOT=$(realpath "$PARENT_DIR"/..)

cd "$IROS_ROOT"

cmake --preset gcc_release_tools
cmake --build --preset gcc_release_tools --target install
"$IROS_ROOT/build/host/gcc/release/tools/install/bin/generate_presets" --prettier --output "$IROS_ROOT/CMakePresets.json"

