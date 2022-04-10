# Build Instructions

## Building Natively

-   To be able run some command lines tools (like the shell or text editor) on your host machine, perform a normal cmake build in the project root.
-   `cmake -B build && cd build && make`
-   The build requires at least GCC version 10 for coroutine support.
-   Note that the system has two optional dependencies when compliing natively:
    -   SDL2 is used to run GUI apps locally.
    -   X11 is used by libclipboard for getting/setting the clipboard contents.
-   Even without these libraries, the command line utilities are fully functional.

## Building the Toolchain

-   Before compiling the system, you need to build the os specific toolchain. This includes binutils, gcc, and cmake.
-   The dependencies for GCC are listed on the [OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler#Preparing_for_the_build), and must be installed.
-   CMake tries to build its dependencies automatically, but its build of libcurl may fail. In that happens, try to install libcurl manually.
-   The default architecture is x86_64, but this can be overriden by setting `OS_2_ARCH=i686`.
-   Run `./scripts/setup.sh`, which will prompt you to build the toolchain and bootstrap the system.
    Note that this must be run from the root directory, or the script will be confused.

## Building the OS

-   The setup script creates one build directory: build*$OS_2_ARCH/, which contains 2 other build directories. The native tools are handled by build*$OS_2_ARCH/native/, while the cross compiled os kernel and userland are built in build_$OS_2_ARCH/os_2/.
-   The system can be fully built by running `make` in the build directory, but this is often uneeded.
-   Most of the time, you should run commands directly in the build\_$OS_2_ARCH/os_2/ directory. A full build is only required when specific tools are modified (namely anything in the gen/ directory).

## Running the OS

-   The OS can be run with either qemu-system-$OS_2_ARCH or bochs. Creating a bootable image requires the grub tools (grub-file and grub-mkrescue). Creating an ext2fs requires parted, mke2fs, losetup, and being able to mount an ext2fs.
-   The OS is booted from an ISO (cdrom) file, which can be created with `make iso`. This contains grub2 as well as the kernel object file, and is output to os_2.iso in the os_2 build directory.
-   The OS additionally must mount a root file system. A suitable hard disk image can be created using `make image`. This command requires sudo becuase it mounts os_2.img directly, and writes to its using regular file system APIs.
-   With both an iso and disk image, the OS can be run using `make run` (for qemu) or `make brun` (for bochs).
-   In addition, the aliases `make frun` and `make bfrun` can be used to fully build the system (image and iso included), and also start an emulator instance.
