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

-   You need to install various dependencies for gcc (look on osdev)
-   Install qemu-system-x86_64 to run the os, also grub-mkrescue
-   Run `./setup.sh`, which will prompt you to build the toolchain and create two cmake build directories
    -   native - for tools that must be run on the host operating system
    -   build - for the os and other cross compiled components
-   Now, you can build the system by typing `cmake --build build`
-   Make the disk image with `sudo ./makeimg.sh`
-   Run the system inside qemu with `./qemu.sh`
-   Alternatively, you can cd into the build directory and type `make`, `make os_2.img`, and `make run`

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
