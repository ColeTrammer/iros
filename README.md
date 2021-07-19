# os_2

My second attempt to create an OS

# Features

-   Multiboot2 boot loader compliant
-   Supports singularly x86_64
-   Memory management system (physical and virtual memory)
-   Preemptive scheduling with SMP support
-   fork and exec
-   vfs implementation
-   many unix system calls
-   IPV4 (UDP + TCP + ICMP) and Unix sockets
-   DHCP client
-   ext2 read/write support (for smaller files)
-   x86 pic devices (PS2 kbd/mouse, PIT, ATA, CMOS, serial ports, local APIC, IO APIC)
-   pci devices (E1000 network card, BGA Device)
-   C standard library
-   Shell that supports the POSIX grammar, job control, word expansion, etc.
-   Some posix standard utilities
-   Basic Desktop enviornment (with themes)

# To build

-   You need to install various dependencies for the gcc (look cross compile on osdev)
-   To build locally, gcc/g++ must have at least version 10 (for coroutine support).
    -   Additionally, libX11 and libSDL2 are needed.
    -   These correspond to the following apt packages: libsdl2-dev gcc-10 g++-10
-   Install qemu-system-x86_64 to run the os, also grub-mkrescue
-   Run `./scripts/setup.sh`, which will prompt you to build the toolchain and create two cmake build directories.
    Note that this must be run from the root directory, or the script will be confused.
    -   native - for tools that must be run on the host operating system
    -   build - for the os and other cross compiled components
-   Now, cd into the build directory and type `make && make install && make iso && make image`.
    This builds the system and installs it to the sysroot directory. Then, a bootable iso file is
    made (containing the kernel as well as grub2), and finally, a ext2 disk image is created which
    contains a union of both 'base' and 'sysroot'.
-   To run with qemu, type `make run`, and to run with bochs, type `make brun`.
-   In addition, the aliases `make frun` and `make bfrun` can be used to fully build the system (image and iso included),
    and also start an emulator instance.

# TODO

-   configuration/startup files
-   pthreads like library (conditions, barriers, cancellation, rdwrlock, scheduling)
-   USB subsystem
-   improved SMP handling
-   more advanced scheduler
-   PATA channels
-   AHCI SATA
-   Hardware agnostic timers
-   Sound subsystem
-   IP v6 networking support
