# Boot
Part of system that executes after the boot loader. It handles jumping
into 64 bit mode and then jumps into kernel main. This also handles
initialization of CPU flags, like enabling the FPU.

# Initrd
Contains a file that builds an Initialization RAMdisk from the files
in the files/ directory, and single file c sources that are used to
test the system. Now that ext2 read/write support exists in the operating
system, this is fairly useless.

# Kernel
Contains code for processing, system calls, etc.

# Libc
Custom implementation of the posix c standard library.

# Ports
Contains scripts and patches of various third party software that
allow it to run on the OS.

# Toolchain
Patches and scripts to build the cross compiler to build the system.

# Userland
Several command line utilities used by the system, including a shell.
