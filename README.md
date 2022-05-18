[![Open in Remote - Containers](https://img.shields.io/static/v1?label=Remote%20-%20Containers&message=Open&color=blue&logo=visualstudiocode)](https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/ColeTrammer/iros)
[![Toolchain](https://github.com/ColeTrammer/iros/actions/workflows/toolchain.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/toolchain.yml)
[![Iros Disk Images](https://github.com/ColeTrammer/iros/actions/workflows/image.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/image.yml)
[![Native Tests](https://github.com/ColeTrammer/iros/actions/workflows/native.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/native.yml)

# Iros

My second attempt to create an OS

# Demos

## Doom

[Doom Demo](https://user-images.githubusercontent.com/18405484/165892090-54040689-8e95-4efe-b29b-d8fb30a0390d.mp4)

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

# Supported Architectures

-   x86 (32 and 64 bit)

# Features

-   Multiboot2 boot loader compliant
-   Memory management system (physical and virtual memory)
-   Preemptive scheduling with SMP support
-   Unix process primitives (fork and exec)
-   Vfs supporting an initrd, ext2, tmpfs, devfs, sockets, and pipes
-   Many posix system calls
-   IPV4 (UDP + TCP + ICMP) and Unix sockets
-   DHCP client
-   x86 PC devices (PS2 kbd/mouse, PIT, ATA, CMOS, isa serial ports, local APIC, IO APIC)
-   PCI devices (E1000 network card, BGA Graphics Device)
-   C standard library, supporting a wide subset of POSIX
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
-   AHCI, SATA
-   Sound subsystem
-   IP v6 networking support
-   Better GUI library
-   Better drawing primities
-   TTF fonts

