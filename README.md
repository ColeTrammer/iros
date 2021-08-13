# os_2

My second attempt to create an OS

# Demos

## OS

[OS Demo](https://user-images.githubusercontent.com/18405484/129427161-d5514538-4a11-4564-96a0-b515ab37c5d6.mp4)

## Shell

[Shell Demo](https://user-images.githubusercontent.com/18405484/129427291-3422c899-cbf2-4e16-9e71-5dd8b72d24fb.mp4)

## Text Editor

[Text Editor Demo](https://user-images.githubusercontent.com/18405484/129427374-c575427e-9653-4a40-90e0-656aae2ba64c.mp4)

## Networking

[Networking Demo](https://user-images.githubusercontent.com/18405484/129427245-08812ca8-698b-4eda-9436-8149e88764e2.mp4)

## GUI

[GUI Demo](https://user-images.githubusercontent.com/18405484/129427196-777ef90a-c22a-4e2c-a5b9-eff8dfaf5365.mp4)

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
-   Portable userland programs which build and run on linux.

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
