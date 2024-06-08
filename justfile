preset := env("PRESET", "clang_release_default")
iros_preset := if preset =~ 'iros' { preset } else { "gcc_iros_x86_64_release_default" }
test := "^test_di$"
default_iros_test := "^test_iris$"

alias c := configure
alias b := build
alias t := test
alias tonly := test_only
alias cb := configure_build
alias bt := build_test
alias cbt := configure_build_test
alias btonly := build_test_only
alias bf := build_file
alias bonly := build_target
alias r := run
alias br := build_run

# Default command: configure and build
default:
    @just preset={{ preset }} configure_build

# Configure
configure *args="":
    cmake --preset {{ preset }} {{ args }}

# Build
build *args="": ensure_configured
    cmake --build --preset {{ preset }} {{ args }}

# Run tests
test *args="": ensure_configured
    ctest --preset {{ preset }} {{ args }}

# Run a specific test (regex matching)
test_only name=test: ensure_configured
    ctest --preset {{ preset }} -R {{ name }}

# Configure and build
configure_build:
    @just preset={{ preset }} configure
    @just preset={{ preset }} build

# Build and test
build_test:
    @just preset={{ preset }} build
    @just preset={{ preset }} test

# Configure and build and test
configure_build_test:
    @just preset={{ preset }} config
    @just preset={{ preset }} bt

# Build and run a specific test (regex matching)
build_test_only name=test:
    @just preset={{ preset }} build
    @just preset={{ preset }} ctest_only {{ name }}

# Compile a specific file (regex matching)
build_file name: ensure_configured
    #!/usr/bin/env bash
    set -euo pipefail

    targets=$(cmake --build --preset {{ preset }}_non_unity -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
    )

    echo -e '\e[1m'"cmake --build --preset {{ preset }}_non_unity -t "$targets'\e[m'
    cmake --build --preset {{ preset }}_non_unity -t ${targets}

# Build a specific target (regex matching)
build_target name: ensure_configured
    #!/usr/bin/env bash
    set -euo pipefail

    targets=$(cmake --build --preset {{ preset }} -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
        | grep -vF '.cxx.o' \
        | grep -vF 'cmake_object_order' \
        | grep -vF 'CMakeFiles' \
        | grep -vF 'CMakeLists.txt' \
        | grep -vF '/install' \
        | grep -vF 'verify_interface_header_sets' \
        | grep -vF '/edit_cache' \
        | grep -vF '/rebuild_cache' \
        | grep -vF '/list_install_components' \
        | grep -vE '/all$' \
        | grep -vE '/test$' \
        | grep -E '/' \
    )

    echo -e '\e[1m'"cmake --build --preset {{ preset }} -t "$targets'\e[m'
    cmake --build --preset {{ preset }} -t $targets

# Run a specific program (regex matching)
run name *args: ensure_configured
    #!/usr/bin/env bash
    set -euo pipefail

    targets=$(cmake --build --preset {{ preset }} -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
        | grep -vF '.cxx.o' \
        | grep -vF 'cmake_object_order' \
        | grep -vF 'CMakeFiles' \
        | grep -vF 'CMakeLists.txt' \
        | grep -vF '/install' \
        | grep -vF 'verify_interface_header_sets' \
        | grep -vF '/edit_cache' \
        | grep -vF '/rebuild_cache' \
        | grep -vF '/list_install_components' \
        | grep -vE '/all$' \
        | grep -vE '/test$' \
        | grep -E '/' \
    )
    build_directory=$( \
        jq -rc '.configurePresets.[] | select(.name == "{{ preset }}") | .binaryDir' < CMakePresets.json | \
        sed s/\${sourceDir}/./g \
    )

    set +e
    for target in $targets; do
        echo -e '\e[1m'"$build_directory/$target {{ args }}"'\e[m'
        $build_directory/$target {{ args }}
    done
    exit 0

# Build and run a specific program (regex matching)
build_run name *args:
    @just preset={{ preset }} build_target {{ name }}
    @just preset={{ preset }} run {{ name }} {{ args }}

alias ic := iros_configure
alias ibimg := iros_build_image
alias ir := iros_run
alias ib := iros_build
alias ibr := iros_build_run
alias it := iros_test
alias itonly := iros_test_only
alias ibt := iros_build_test
alias ibtonly := iros_build_test_only
alias ibd := iros_build_debug

# Configure the build system for Iros
iros_configure:
    cmake --preset {{ iros_preset }}

# Build Iros disk image
iros_build_image: ensure_iros_configured
    cmake --build --preset {{ iros_preset }} -t image

# Run Iros
iros_run: ensure_iros_configured
    cmake --build --preset {{ iros_preset }} -t run

# Full build Iros and produce disk image
iros_build: ensure_iros_configured
    cmake --build --preset {{ iros_preset }}

# Full build Iros and run
iros_build_run: ensure_iros_configured
    cmake --build --preset {{ iros_preset }} -t ibr

# Run tests on Iros
iros_test: ensure_iros_configured
    ctest --preset {{ iros_preset }}

# Run a specific test on Iros (regex matching)
iros_test_only name=default_iros_test: ensure_iros_configured
    ctest --preset {{ iros_preset }} -R {{ name }}

# Full build Iros and run tests
iros_build_test:
    @just iros_preset={{ iros_preset }} ib
    @just iros_preset={{ iros_preset }} it

# Full build Iros and run a specific test (regex matching)
iros_build_test_only name=default_iros_test:
    @just iros_preset={{ iros_preset }} ib
    @just iros_preset={{ iros_preset }} itonly {{ name }}

# Run Iris kernel in debug mode
iros_build_debug iros_preset="gcc_iros_x86_64_release_iris_debug":
    @just iros_preset={{ iros_preset }} ibimg
    IROS_DEBUG=1 IROS_DISABLE_KVM=1 cmake --build --preset {{ iros_preset }} -t run

# Build Iros cross compiler
build_toolchain:
    ./meta/toolchain/build.sh

# Build Iros toolchain docker image
build_docker:
    docker build -t ghcr.io/coletrammer/iros_toolchain:iris . -f meta/docker/Dockerfile

# Generate the CMakePrests.json file
generate_presets:
    @just preset=gcc_release_tools build_run generate_presets -p

# Auto-format source code
format:
    nix fmt

# Validate format and lint code
check:
    nix flake check

# Build docs
build_docs: ensure_configured
    cmake --build --preset {{ preset }} --target docs

# Open docs
open_docs: ensure_configured
    cmake --build --preset {{ preset }} --target open_docs

# Build and open docs
docs:
    @just preset={{ preset }} build_docs
    @just preset={{ preset }} open_docs

# Select a CMake preset (meant to be run with eval, e.g. `eval $(just choose)`)
choose:
    @echo "export PRESET=\$(cmake --list-presets=configure | tail +2 | fzf | awk '{ print \$1 }' | tr -d '[\"]')"

[private]
ensure_configured preset=preset:
    #!/usr/bin/env bash
    set -euo pipefail

    build_directory=$( \
        jq -rc '.configurePresets.[] | select(.name == "{{ preset }}") | .binaryDir' < CMakePresets.json | \
        sed s/\${sourceDir}/./g \
    )
    if [ ! -d "$build_directory" ]; then
        echo -e '\e[1m'"cmake --preset {{ preset }}"'\e[m'
        cmake --preset {{ preset }}
    fi

    build_directory="$(realpath $build_directory)"
    if [ "`readlink build/compile_commands.json`" != "$build_directory"/compile_commands.json ]; then
        rm -f build/compile_commands.json
        ln -s "$build_directory"/compile_commands.json build/compile_commands.json
    fi

[private]
ensure_iros_configured:
    @just ensure_configured {{ iros_preset }}
