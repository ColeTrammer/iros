# Build Instructions

## Building Iros
-   Compiling the system requires an os specific gcc toolchain and several dependencies. See the project's root Dockerfile for a full list of requirements, but there is no need to install them locally.
-   Producing a docker image capable of developing the system can be done by running `docker build -t iros_toolchain .`, but this is not required, since an up-to-date Docker image is available as `ghcr.io/ColeTrammer/iros_toolchain`. This is built and published automatically in CI whenever the toolchain files change.
-   Inside the docker container, all that is needed is to run `IROS_ARCH=<x86_64|i686> ./scripts/setup.sh`, and then run `ninja` in the build directory.
-   Since the system depends on various code generators, the build directory contains 2 relevant sub builds. `native` compiles
    only these code generators, while `iros` contains the actual build for the operating system. Once `native` is compiled once, there's usually no reason
    to touch it, and you run `ninja` commands directly in the `iros` folder of the build.
-   Building a disk image is possible by running `ninja image` in the `iros` subdirectory of the build. The shortcut target `bi` is provided to both fully build the system and produce a disk image.

## Development using VS Code Remote Container Extension
-   Since all the dependencies are nicely in a Dockerfile, it is best to develop the system using VS Code's remote container support. The provided
    .devcontainer directory makes this extremely easy to use.
-   However, since the container won't be connected directly to any GUI, Iros can't run in graphical mode inside the container. Instead, after
    creating build images, run `IROS_ARCH=<x86_64|i686> ./qemu.sh` in the `scripts/` directory on your local machine. This requires install qemu if it is not already present.
-   This can be worked around by trying to mount the X11 socker directly into the container, but is rather complicated due to X11 authorization.

## Building Natively

-   To be able run some command lines tools (like the shell or text editor) on your host machine, perform a normal cmake build in the project root.
-   `cmake -B build_native -G Ninja -S . && cd build_native && ninja`
-   The build requires at least GCC version 10 for coroutine support.
-   Note that the system has two optional dependencies when compliing natively:
    -   SDL2 is used to run GUI apps locally.
    -   X11 is used by libclipboard for getting/setting the clipboard contents.
-   Even without these libraries, the command line utilities are fully functional.
