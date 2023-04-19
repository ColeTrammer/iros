# Development Plan

Constructing an functioning kernel requires a large amount of code and development effort. Furthermore, the point at
which development feels rewarding is when the kernel is capable of running userspace applications. This makes
progressing development challenging, as it takes a long time before the system performs a meaningful task.

## Phase 1: Minimal Kernel

The initial development effort is to create a minimal kernel which can run a userspace program. Additionally, core
kernel functionality which is hard to retroactively must be put into place. This includes security measures and multi
processing support (SMP).

### Minimal Steps Needed to Run a Program (x86_64)

For the minimal case, we can skip pre-emptive scheduling, file system IO, and executable loading by using co-operative
scheduling and executing a kernel function directly in ring 3.

1. Get control of the CPU
2. Setup the GDT and TSS
3. Create a list of task objects in the kernel
4. Context switch to a newly created task

### Next Steps for Running a Simple Program

The above steps allow ring 3 code to execute, but the code cannot really be useful. However, the program should be able
to print to the serial port if the kernel relaxes the restrictions on IO port usage. However, a system call interface
needs to be added so support subsequent development, to perform things like memory allocation and yeilding execution.

1. Basic system calls (syscall instruction) for printing

### Steps Completeable in Isolation

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

### Note on Microkernel Architecture

For the initial period of development, the intention to create a microkernel should be ignored. This is simply because
separate processes are impossible when the kernel itself does not support processes. Ideally, the kernel will have a
sensible user-facing API, which would allow the internal implementation of kernel components to slowly be moved into
separate processes.

### Overall Development Intent

The number one goal should be to run a userspace program properly. Until acheiving this goal, the kernel code is allowed
to be poorly architected and thought-out. All that matter is getting to a point where the code actually does something.
At this point, the code should be refactored, which making sure it is still functioning.

## Phase 2: Full Vertical Slices

Once the kernel is capable of running a userspace program, and has some core solid-ish foundations, the next step is to
add the remaining components. This will require refactoring and a lot of combined work between kernel and userspace. For
example, the kernel will need to implement filesystem support via a VFS. Userspace will need to support consuming
abstractions, the libc must provide POSIX compatibility, and userspace must also implement concrete filesystem drivers.
Ideally, userspace programs for creating and inspecting disk images should be added as well. This is just one example,
and there are many other components which will need to be implemented.

### Vertical Slices

- File system
- Memory management
- Process management
- Time keeping
- Dynamic linking
- Networking
- Graphics
- Audio
- Syncronization
- Containerization

### Horizontal Slices

For constrast, horizontal slices refer the individual components themselves. The idea is that each vertical slice is
part of expanding multiple horizontal slices, which effectively means that the whole system becomes more usable. For
instance, POSIX compatibility is a horizontal slice, and different system calls fall into different vertical slices.

- POSIX compatibility
- Dius userspace library
- Iris kernel
- Userspace applications

### File System

The file system is the heart of modern operating systems. In many systems, every other API is effectively built on top
of the file system. At the same time, the file system API determines the performance and usability of the system.
Without a good page caching mechanism, the OS will be painfully slow in comparison to other systems. Unfortunately,
there is a lot that needs to be done to create a good VFS API, and exposing this to userspace will require a lot of
thinking.

### Memory Management

Memory management and the file system are somewhat intertwined, especially considering concepts such as POSIX mmap() and
the potential need to swap out memory. Although memory management is a bit less complicated, as it will probably not be
mostly done in userspace, it still requires a lot of refactoring. Currently, the kernel can really only immediately map
pages, and doesn't support lazy allocation, copy-on-write, or swapping. And privledged kernel-space APIs will need to be
added to implement certain drivers in userspace.

### Process Management

Process management is important for POSIX compatibility and the shell. It is especially unclear what abstractions the
kernel should expose, because the goal is not implement POSIX directly, but to have userspace be able to effectively
emulate it. Of particular note is the need to implement fork() and POSIX signals.

### Time Keeping

Time keeping is comparatively simple, but there are additional drivers the kernel needs to implement (RTC, HPET, etc).
Userspace must expose this API, which involves working on the chrono library component, and POSIX compatibility. And
some timer mechanism needs to be implemented.

### Dynamic Linking

Dynamic linking requires some work in the kernel, to support ELF interpreters. However, the bulk of the work is in the
dynamic linker, but libc also needs to be updated to support it.

### Networking

Networking is actually one of most execiting components to implement. The kernel will need to implement a network stack,
and userspace needs to be able to consume it. With the focus asynchronous IO, the hope is to be able to implement a
performant system. It would be exciting to compare this implementation on between Linux and Iros.

### Graphics

Graphics is likely to be very difficult. Most likely, the goal should be to port X11 or Wayland to the system. This will
involves lot of POSIX compatibility work, and a choice must be made as to whether to support the Linux DRM API or to
port these programs to some other custom API.

### Audio

Audio is a lot approachable than graphics, but I don't know much about it. The kernel will need to have some sort of API
and support audio drivers, and userspace will need to implement a audio server. Hypothetically, porting some Linux
solution could also be done.

### Syncronization Primitives

Synchonization primitives are important for applications and the pthreads library. The kernel will need to implement
either `futex` or some other mechanism for implementing these sorts of things.

### Containerization

Containerization is a bit of a stretch goal, but it would be nice to have. The kernel will need to implement some sort
of virtualization mechanism, and userspace will need to implement a container runtime. Look at Linux KVM for
inspiration.
