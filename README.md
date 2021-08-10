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

-   [See this document](docs/build.md)

# TODO

-   configuration/startup files
-   USB subsystem
-   improved SMP handling
-   more advanced scheduler
-   AHCI SATA
-   Sound subsystem
-   IP v6 networking support
