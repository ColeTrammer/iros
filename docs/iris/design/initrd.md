# Initrd

## Explanation

In a kernel with modules, there needs to be some way to load the initial kernel modules needed to boot the system. These
include disk drivers and file system drivers which allow the kernel to fully initialize itself. To bootstrap this
process, there needs to be a way to load modules easily.

The standard solution is the "initrd", or initial ramdisk, which is a small read-only file system which is loaded into
main memory by the bootloader. Since this file system is readonly, it's design can be a lot simpler than "real" file
systems.

An additional benefit of using an "initrd" is that it can serve a first file system for the kernel, allowing for the
initial development of a VFS layer.

## Format

The initrd used by the Iris kernel is a custom format, which focuses on simplicity above all else.

The overall design is to break the initrd into 4 KiB blocks (page-sized). The first block contains the root directory
entry and a signature, which identifies that the file system is formatted properly. This serves as a sanity check during
kernel boot.

Directories and files store their content directly in a single contiguous range of blocks. Their metadata is stored in
their corresponding directory entry. Under this system, hard links are not supported.

Directories store directory entries, which contain the necessary metadata to load a file, as well as its name. These
entries are variable sized, but are always 8 byte aligned and never cross a block boundary.

## Design Diagram

![](/docs/assets/initrd.drawio.svg)
