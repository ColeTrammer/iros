# Kernel

# arch
This directory contains all the architecture specific code
Currently, this includes parsing sys call input, handling
irqs, save context state, using virtual memory, and the
establishment of a valid C enviornment on boot.

ALso, only x86_64 is supported, but i386 should be able
to be added very easily. Other achitectures would be much
more difficult.

# fs
This directory contains all code related to the file
system. The general segments are the inode_start.c, which
is a hash map that stores inodes by device and ino_t, 
tnode.c, which provides methods for dealing with tnodes
(store name of an inode in list form (the list is the directory)),
and vfs.c, which is the generic virtual file system implementation.
There is 1 disk file system supported, ext2, and also file systems
that aren't disk backed, like device, initrd, pipe, and tmp file
systems. The initrd is essentially useless at this point, put the
other ones are useful. Note, sockets are currently implemented
not as a file system, but instead in the net directory by overriding
the file_operations structure, but this may need to be changed.

The the only file system that needs to be added currently is some
form of a procfs, and the ext2 driver should be expanded and eventually
upgraded to have more advanced features in later revisions of the file
system.

# hal
The hardware abstraction layer provides device drivers and a
kernel level abstraction over different devices. This includes
the code for generic devices (like /dev/null and /dev/zero (/dev/random
and /dev/urandom need to be added)), and the ptmx implementation.
Architecture specific code deals with x86_64 specific things, like the gdt
and idt, and provides a unified way to perform debug output and timer
specific things. Currently supported devices are ata hard drives,
bochs/vm graphics card, cmos, e1000 network card, fdc (this isn't
actually implemented, its just a stub), ps2 keyboard/mouse, simple
pci control, programmable interrupt controller, pit, serial port,
and basic vga driver.

Work needs to be done on the timer interface. In terms of devices,
APIC is probably the most important, but USB support is also
important.

# main
This contains one source file, kernel.c which does initialization
of the system and not much else.

# mem
Subsystem for dealing with memory allocation. The most robust and
functional part of this code is the page frame allocator, which
keeps track of physical pages being used in a large bitmap.
The vm allocation and vm region objects are somewhat lacking
in functionality (which is very muched needed for mmap MAP_ANON
and normal mmap). They currently are only good for keeping track
of the process address space, but there's no way to add things
once a program is already loaded (that's the main idea).

# net
The network implementation. Supports AF_UNIX and AF_INET. AF_UNIX
only works with SOCK_STREAM, but why would anyone use SOCK_DATAGRAM
anyway, right... Anyway, the inet code supports raw sockets (at
least for icmp), datagram sockets with UDP, and preliminary TCP
support. It can perform simple functions correctly, like make a
HTTP 1.0 request, but only in ideal conditions. There are issues
around needing to wait for an ack when sending (if SOCK_NONBLOCK
isn't set), and also it needs to resend packets if the server
doesn't ack properly. Also, probably should respect the default
congestion control in TCP which makes clients wait like 100-500ms
before sending anything.

# proc
Code for dealing with tasks and processes. Supports loading
elf64 execuatables and performing some form of a simple stack
trace (it just reads everything on the stack, it doesn't read
the stack frames). Currently, threads are not supported as the
code base was just refactored to separate the notion of processes
from tasks/threads (linux calls them tasks, but they're threads).
Right now, process_state.c stores the information received from
waitpid/times (eventually should be waitdid and getrusage). This
code also handles signals, which are implemented with a trampline.
Getting signals to work right is basically impossible, since
signals can effectively be nested indefinately, so they can't
use different kernel tasks. This means you can't save any state
on the stack. This is pretty much not solves at all, we just
return EINTR on any sys calls (unless it was sent by itself).
This means that any memory malloced during this sys call is not
freed! The way to fix this I think is to have check points where
signals can interrupt the sys calls somehow. Also, threading supports
needs to be implemented, but this can't be too hard as long as
tasks properly share process information (although this may
lead to various concurrency issues).

# sched
This is the basic scheduler implemention. It is the simplest possible
method, a round robin queue with no priority levels at all. Also, it
runs the idle task in this queue instead of only when no threads
are asleep, which is inefficent. It also dispatches signals.

# util
This has general code necessary for the kernel to function. There
is currently only two things in here: spinlock and hash_map, both
of which are crucial for the code. Spinlocks should definately be
replaced by mutexes at some point (only for certain locks of course),
and the hash_map implemenation doesn't even support resiving, has
only a single table wide lock, and is probably completely broken for
some unexplored input. Nevertheless, it is still very important
to the code base.
