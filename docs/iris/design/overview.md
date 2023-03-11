# Iris Kernel Overview

# Reason for Writing a New Kernel for Iros

The main question here is why abandon development of the old kernel written in C for a new one. There are 2 primary
reasons:

1. The old kernel was unmaintainable, and C++'s ability to add safe abstractions will improve the situtation. For one,
   RAII and the Result class make handling errors without leaking resources effectively trivial. Smart pointers should
   make memory safety bugs significantly less likely, and class templates like di::Synchronized should make
   thread-safety simpler.
2. The old kernel was modelled strictly off of POSIX, which includes an extremely large body of obsolete and duplicate
   functionality. Writing a kernel with asynchronicity in mind, and leaving userspace to emulate POSIX APIs, many of
   which are duplicated (think SYS-V semaphores vs. POSIX semaphores), will result in a much cleaner design.

The current issues with the old kernel are mostly around poor handling of error conditions and frequent concurrency
issues, which make the system very unstable when trying to do something like run GCC. Due to performance issues in the
kernel, running GCC takes at least 10x longer than it would on linux, and that is only considering compiling a single
file. Attempting to compile the entire code base would would most likely take hours, but GCC would crash first anyway,
even on a single core processor.

There are also several missing features, like support for USB, newer hardware platforms like AHCI, or nvme, IP v6
networking, or even any form of audio playback. The benefits of using a microkernel architecture are that prototyping
these sorts of drivers can be done in userspace, which makes common concerns like thread-safety, handling error
conditions, and avoiding memory allocations at all cost, much less problematic.

If a new AHCI driver crashes, the system will still be operational and the driver can simply be restarted. Thread-safety
is irrelevant if the driver only has 1 thread, and only communicates with the kernel through a pair of SPSC queues.
Hypothetically, a userspace program could actually be debugged using gdb, without freezing the entire system. This will
make developing this functionality way more managable.

Ultimately, the goal of this new kernel is to acheive good performance while also making development of new features
easier.

# Design Priorities of the Iris Kernel

1. Asyncrounous
2. Minimal
3. Performant

## Aysynchronous

By default, every system call should be non-blocking. Ideally, there will be exactly one mechanism by which processes
can block themselves, and no others.

## Minimal

For security and robustness, as few components as possible should reside within the kernel. Therefore, the kernel will
only handle scheduling, memory management, and direct hardware access.

## Performant

The core IPC mechanism needs to be fast and scalable. This minimizes any performance loss caused by using a
micro-kernel.

# Micro-Kernel vs. Monolithic Kernel

In a "classical" monolithic kernel, all supervisor code, including all drivers, runs in ring 0 in the same singlular
address space. In a micro-kernel architecture, as many components as possible are moved into separate processes, most
hopefully running in ring 3.

## Pros of a Micro-Kernel

-   Increased fault tolerance (if the NIC driver crashes, the system can keep running)
-   Increased security (it is much harder to exploit components not directly in the kernel)
-   Increased modularity (different components can be developed in isolation)

## Cons of a Micro-Kernel

-   Increased complexity (now, multiple different processes must orchestrate themselves together)
-   Relies on Inter-Process Communication (not the much simpler interface, function calls)
-   Reduced performance (more context switches, non-blocking APIs, IRQ dispatch, ...)
-   Requires complex work-arounds (like bootstrapping a file system so that programs can be loaded)

## Pros of a Monolithic Kernel

-   Components communicate directly using function calls
-   Increased performance (component communication has no overhead)

## Cons of a Monolithic Kernel

-   Large attack surface
-   Failures in less significant components can cause the entire kernel to panic

## General Analysis

It is generally held that a micro-kernel architecture is superior, if doing so is actually viable (including achieving
comparable performance). This quest has resulted in large amounts of academic research investigating the subject.

Sometimes, the performance loss caused by requiring IPC to communicate with the kernel can be mitigated by having the
kernel execute a verifiably safe program provided by said component.

This technique is used by microkernels to handle hardware IRQs without waiting for a response from the underlying device
driver. The driver only needs to provide a simple program which convinces the device the interrupt has been received.
Then, the kernel arranges for the driver to be scheduled, at which point actual work will be done (e.g., notifying
another component the disk read was successful). This 2 phase approach to interrupt handling can be considered to the
Linux soft IRQ mechanism. Additionally, hybrid-poll based drivers can disable interrupts when the device is known to
actively be doing work, and instead continually poll the device.

This technique is additionally not exclusive to micro-kernels. The eBPF feature of Linux allows userspace code to
control program executing in kernel context. This has numerous use cases, including in AF_XDP, where it is used to
filter incoming packets before they are handed off to higher layers of the network stack. Using this functionality, the
network protocols (e.g. IP v6, TCP, TLS, etc.) can all be implemented directly in userspace.

## File System Analysis

Linux also provides the FUSE API for implementing file system drivers in userspace. This is considered valuable for
prototyping file system implementations, but file system drivers are usually converted into kernel space versions, in
order to achieve supererior performance.

A pure userspace file system implementation faces numerous challenges which make obtaining comparable performance
difficult.

The first relevant issue is the so-called "page cache". There must be a centralized cache of memory to minimize memory
usage and enable any application to access this cache. Therefore, the kernel must be able to manage the page cache
entries with the helper of file system drivers (for example, to flush dirty pages out to disk).

Along with the "page cache", the results of path lookups should also be cached. Using Linux, the dirent cache is
consulted along every step of the traversal. For optimal performance, this lookup must be done by a central authority.
The result of path lookup is further complicated by the mutable nature of the file system. When a file is deleted, the
central authority must be made aware of this deleting to not end up with a stale cache.

Another notable problem is that of frequent context switches. When a userspace program requests to read 2048 bytes from
a file, they would likely make a request to some central authority. This request then gets routed to the proper
userspace driver (if the requested page is not in the "page cache"). At this point, the device driver must locate the
disk sector which contains the relevant data. In the simply case, this can be done without any communication with the
disk driver. However, actually reading the data requires calling into the disk driver. Finally, when the disk driver
receives a completion event, the chain unravels, passing through the device driver into the cetral authority and then
back the requesting application. On a single-processor system, this requires at least 4 context switches.

However, imagine instead this scenario with a 16 core machine that is actively reading an entire 32 MiB file. When doing
sufficently large amounts of IO, the effective operation cost be bounded by the speed of the disk itself. To read the 32
MiB file as quickly as possible, it is enough to ensure the disk's queue of read requests is always full. During this
scenario, the separate processes will run on separate cores, communicating with each other through lock-less SPSC queues
(think Linux io_uring). In this scenario, throughput is maximized, since the file system driver can submit multiple
blocks to be read at once.

Notice, this requires the underlying application uses asynchronous IO itself, will allows it to drive more and more
requests for data. When using a synchronous IO, the kernel would have to specutively prefetch subsequent sectors to
acheive a similar effect.

The conclusion is thus that asynchronous IO can tolerate the increased context switches readily, and although there is
higher latency when the "page cache" is missed, throughput should be unaffected.

## IPC Wake Up

Another serious issue to consider is the mechanism to wake tasks which have received requests over the IPC mechanism. In
Linux io_uring, a system call (io_uring_enter) is used to notify the kernel there is work to do. Alternatively, an
application can request the kernel busy-poll its submission queue for new entries. Note this assumes that the IPC is
purely asynchronous.

In a multi-processing scenario, the responsible task could already be awake and busy, and thus the calling program needs
not wakeup anyone. However, it is equally possible that the task needs to be awoken. To do this, the kernel itself must
perform some sort of mediation. This could be implemented by calling into the kernel, which would then attempt to
schedule the relevant task, before returning to the caller.

It would be nice if the caller could directly call into the responsible task (using an interrupt vector), but this
cannot be considered scalable (given that only 255 vectors are available). More research would be needed to determine
the optimal mechanism for this case.
