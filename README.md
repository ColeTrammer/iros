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
* x86 pic devices (PS2 kbd, PIT, ATA, CMOS, serial ports)
* C standard library that's becoming fleshes out
* Shell that supports job control and pipes and lists and globbing
* Start of posix standard utilities

# TODO
* Shell supporting if, for, etc
* Networking
* Graphics
* mmap in kernel
* configuration/startup files
* SMP
* pthreads like library
* try to write in C++ instead of C

# Current Issues
* When a signal handler interrupts a sys call, the sys call will leak
  memory if it ever called malloc, since the signal discards the entire
  state of the sys call. A possibly solution is to allocate things to the
  stack instead.
* Tty driver will in query the keyboard when being read from, so a user cannot press
  ^C most of the time. To fix this, a master and slave tty mechanism must be used, 
  allowing the master tty to use it's time splice to read from the keyboard, and thus
  allowing the input to be read asynchronosouly with the running process.
* Unix permission support is extremely limited, nothing has uids or gids, and the
  execute but is never respected.
* Created files can only be read by this OS, not any others. This is probably an issue
  in block allocation that causes the inode's data blocks to not be marked as used, but
  I do not know for certain.
* Signal handling is in general somewhat unreliable, meaning that it can cause random
  issues when be enabled.
* FPU state cannot be saved- only 1 process can use it at a time or we just assert and exit.
  This is because the the fpu stack must be 16 bytes aligned, but it is currently random.
* Neither GCC not Bin Utils want to build anymore, something must be wrong with the supplied
  patches.
* Ports cannot be built all at once: the script doesn't work because it needs to set the CWD
  correctly, so each port must be built individually.
* make run always rebuilds the disk image, so we can never save info in between sessions.
