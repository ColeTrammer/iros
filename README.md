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
* Run `./setup.sh`, which will prompt you to build the toolchain and create two cmake build directories
  native - for tools that must be run on the host operating system
  build - for the os and other cross compiled components
* Now, you can build the system by typing `cmake --build build`
* Make the disk image with `sudo ./makeimg.sh`
* Run the system inside qemu with `./qemu.sh`

# TODO
* Networking (TCP resending)
* Graphics (windowing support)
* configuration/startup files
* SMP
* pthreads like library (conditions, barriers, cancellation, rdwrlock, scheduling)
* try to write in C++ instead of C

# Current Issues
* Unix permission support is extremely limited, nothing has uids or gids, and the
  execute but is never respected.
* make run always rebuilds the disk image, so we can never save info in between sessions.
