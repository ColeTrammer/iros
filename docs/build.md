# Build Instructions

## Using provided dev container

The easiest way to build and run Iros is to use the provided dev container. The CI pipeline for the repository builds
and tests the toolchain image, which ensures everything will work inside the dev container.

To use the dev container, use Visual Studio Code and install the remote contains extension. VS Code will then prompt you
to open the repository inside the dev container. This will also automatically install relevant VS Code extensions, which
enables building and running tests through the VS Code CMake extension.

## Build the toolchain locally

In order to use something other than VS Code, it is probably easiest to just build the toolchain locally. Once finished,
the project can be built like a normal CMake project, and intellisense can be provided using `compile_commands.json` and
any c++ language server. Unfortunately, `clangd` may not be happy with the project, as it uses bleeding-edge c++
features which `clangd` may not support.

In addition, the project requires at least GCC 12.1.0 and the newest version of CMake to compile. The CMake version can
probably be relaxed, but GCC 11 will fail to compile the system. Testing the kernel additionally requires various system
commands, including `parted`, `mkfs.fat`, and `qemu`. A full list of packages needed under Ubuntu can be found in the
[dockerfile](/meta/docker/Dockerfile). This dockerfile also provides steps which install the latest version of CMake on
Ubuntu. For other Linux distros, you will have to look up the corresponding package names for your distro. At the very
least, this list of dependencies is actively tested in CI, and so will always be up to date.

```sh
# Build the toolchain (this will probably take a while)
./meta/toolchain/build.sh

# Add $PROJECT_ROOT/cross/bin to your path.
export PATH="$(realpath .)/cross/bin:$PATH"
```

At this point, the entire system should be buildable with cmake.

### Build Commands

Note that these commands apply using a dev container or locally, once things are setup.

#### Configure

```
cmake --preset iros_x86_64
```

#### Build

```
cmake --build --preset iros_x86_64
```

#### Run Tests

```
ctest --preset iros_x86_64
```

#### Run the kernel directly

```
cmake --build --preset iros_x86_64 --target ibr

# Or alternatively in the build/x86_64 directory.
ninja ibr
```

### Linux Presets

Additionally, presets are defined for compiling the userland libraries and their unit tests on a Linux system. A list of
presets should be displayed by the IDE, or can discovered in the `CMakePresets.json` file. These can be useful for
debugging purposes, especially because we can use sanitizers directly. For instance, the `ubasan` preset compiles the
userspace code with both `ubsan` and `asan` enabled. This configuration is actively tested in CI.

These can be built and used without any special setup, using normal CMake commands, although recent versions of GCC and
CMake are needed.
