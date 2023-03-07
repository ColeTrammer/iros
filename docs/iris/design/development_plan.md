# Development Plan

## Problem

Constructing an functioning kernel requires a large amount of code and development effort. Furthermore, the point at
which development feels rewarding is when the kernel is capable of running userspace applications. This makes
progressing development challenging, as it takes a long time before the system performs a meaningful task.

## Minimal Steps Needed to Run a Program (x86_64)

For the minimal case, we can skip pre-emptive scheduling, file system IO, and executable loading by using co-operative
scheduling and executing a kernel function directly in ring 3.

1. Get control of the CPU
2. Setup the GDT and TSS
3. Create a list of task objects in the kernel
4. Context switch to a newly created task

## Next Steps for Running a Simple Program

The above steps allow ring 3 code to execute, but the code cannot really be useful. However, the program should be able
to print to the serial port if the kernel relaxes the restrictions on IO port usage. However, a system call interface
needs to be added so support subsequent development, to perform things like memory allocation and yeilding execution.

1. Basic system calls (syscall instruction) for printing

## Steps Completeable in Isolation

There are several more components which are needed to have a properly functioning kernel, but these can be carried out
independently.

1. Setup the IDT to enable coding device drivers
2. Setup HW timer to keep track of time
3. Preemptive scheduling [requires IDT and HW timer driver]
4. Virtual memory management (mmap() system call)
5. Block device driver (floppy, PATA, AHCI, or, nvme)
6. File system driver (ext2, FAT, initrd) [requires block driver]
7. Executable loading (ELF) [requires file system or boot loader module]
8. Userspace runtime library (to make system calls)

The end result of completing these items would be a kernel capable of running basic programs. Note that some steps can
be completed in isolation but will likely need refactoring if done in the incorrect order. For instance, the file system
driver can be developed before deciding on a proper VFS API, but it would then need to be changed later. Likewise, an
executable loader which accesses the file system driver would need to change as well.

However, this is acceptable because the VFS API is extremely important, and will likely be influcenced by kernel
architecture decisions (separate processes per fs as well as asnchronous first API).

This means that individual components created during this time period should be as minimal as possible.

## Note on Microkernel Architecture

For the initial period of development, the intention to create a microkernel should be ignored. This is simply because
separate processes are impossible when the kernel itself does not support processes. Ideally, the kernel will have a
sensible user-facing API, which would allow the internal implementation of kernel components to slowly be moved into
separate processes.

## Overall Development Intent

The number one goal should be to run a userspace program properly. Until acheiving this goal, the kernel code is allowed
to be poorly architected and thought-out. All that matter is getting to a point where the code actually does something.
At this point, the code should be refactored, which making sure it is still functioning.
