# os_2
My second attempt to create an OS

# Features
* Multiboot 2 loading
* x86_64
* Memory management system
* Preemptive scheduling
* fork and exec
* vfs implementation
* ext2 read/write support (for smaller files)
* x86 pic devices (PS2 kbd/mouse, PIT, ATA, CMOS, serial ports)
* pci devices (E1000 network card, BGA Device)
* C standard library that's becoming fleshes out
* Shell that supports job control and pipes and lists and globbing
* Start of posix standard utilities

# To build
* You need to install various dependencies for gcc (look on osdev)
* Install qemu-system-x86_64 to run the os, also grub-mkrescue
* Add the output of ./path.sh when inside the toolchain directory to your path
* Run ./build.sh from within the toolchain directory
* Run ./build.sh from within the ports directory (if you want ports)
* Run `make run` within the top level directory

# TODO
* Shell supporting if, for, etc
* Networking (TCP resending)
* Graphics (windowing support)
* mmap in kernel (now just need support for files)
* configuration/startup files
* SMP
* pthreads like library (conditions, barriers, cancellation, rdwrlock, scheduling)
* try to write in C++ instead of C

# Current Issues
* When a signal handler interrupts a sys call, the sys call will leak
  memory if it ever called malloc, since the signal discards the entire
  state of the sys call. A possibly solution is to allocate things to the
  stack instead. We should only deliver signals when we block, and the kernel
  should be controlling when the signal is delivered to prevent leaks.
* Unix permission support is extremely limited, nothing has uids or gids, and the
  execute but is never respected.
* Created files can only be read by this OS, not any others. This is probably an issue
  in block allocation that causes the inode's data blocks to not be marked as used, but
  I do not know for certain.
* Signal handling is in general somewhat unreliable, meaning that it can cause random
  issues when be enabled.
* Neither GCC not Bin Utils want to build anymore, something must be wrong with the supplied
  patches.
* make run always rebuilds the disk image, so we can never save info in between sessions.
