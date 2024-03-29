# Memory Allocation

In general, memory allocation consists of 3 components:

- Physical Page Frame Allocator
- Virtual Memory Allocator (Page Granularity)
- Heap Management (Byte Granularity)

## Physical Page Frame Allocator

There a number of possible methods for allocating physical pages. The main mechanisms which should be considered are the
following:

1. Free-List Allocator
2. Bitmap Allocator
3. Buddy Allocator

### Free-List Allocator

Using a free-list allocator, physical pages can be allocated and deallocated in O(1) time, and no extra space is needed.
At first glance, this would seem to be the optimal solution. However, constructing the free-list at startup is
expensive, since every free-page of memory must be written into. Additionally, there is no way to allocate physically
consecutive pages with this allocation scheme. This can make writing efficent device drivers difficult to impossible,
and prevents the use of huge pages.

### Bitmap Allocator

The classical solution to this problem is the bitmap allocator. Simply assign a single bit which keeps track of the
usage of each physical page. This minimizes the information necessary to track physical pages, but has a worst case
allocation performance of O(N). Additionally, it is inefficent to allocate consecutive physical pages with this scheme.

A typical optimization to this scheme is to remember the bit position of the last allocated page. This can greatly
reduce the amount of linear scanning required when allocating many pages. However, this optimization is ineffective
after the physical memory becomes sufficently fragmented, and the system is under high-memory usage.

### Buddy Allocator

This allocation scheme is used by the Linux kernel, and is similiar in the bitmap allocator except it maintains extra
state about the physical memory, and a coarser granularity than a single page. Thus, to allocate 4 consecutive pages,
the allocator can consult the "buddy" bitmap for free 16K pages. This greatly improves physical memory allocation under
this circumstance.

### In the Iris Kernel

The initial implementation will use a bitmap allocator. After fleshing out the driver interface, and determining that
allocating specicial chunks of physical memory is required, a switch to a buddy allocator can be made.

### Note on Boot Strapping

Another problem in physical page frame allocation is acquiring the necessary memory to track physical pages on boot. A
priori, the OS does not know how much physical memory will be present on the system. In fact, some high-end servers can
have TiB's of memory presenting itself as RAM.

Therefore, the physical memory manager must be bootstrapped in 2 phases. In the first phage, a small amount of static
storage is used to construct the necessary information to track the first 1 GiB of physical memory. Then, virtual and
heap allocators can be initialized. At this point, a second-pass must be made which dynamically allocates the structures
used to keep track of the rest of available memory.

### Note on Physical Memory Management Initialization

In order to not hand out important physical pages when allocations are requested, the kernel relies on receiving a
memory map structure from the boot loader, which contains information on which areas of physical memory are actually
usable. Using this information, the memory manager can be initialized.

### Note on NUMA (Non-Uniform Memory Access)

In certain environments, different regions of physical memory can be accessed at different speeds depending on the
currently executing logical processor. In such cases, the physical memory allocator aims to hand-out physical pages
which are most suitable for access on the currently executing processor.

## Virtual Memory Allocator (Page Granularity)

The Virtual Memory Allocator is responsible for managing individual process's address spaces. Virtual Memory can be
viewed a series of page aligned ranges, each of which corresponds to a differnent "backing" memory object.

```
+-----------------+
|  Program Stack  |         32 KiB @ 0xFFFFFFF80000000
+-----------------+
        ...
+-----------------+
|  Program Heap   |         64 MiB @ 0x000000001000000
+-----------------+
        ...
+-----------------+
|  Program Code   |          2 MiB @ 0x000000000400000
+-----------------+
```

The "program stack" and "program heap" are backed by anonymous memory objects, which are not attached to the file
system. However, the "program code" is loaded directly from disk, and in cases that require minimal relocation, can be
executed directly after reading from disk. In such cases, the backing memory for the "program code" region is not
anonymous, and is actually the same as would be used by the "page cache" to speed up disk reads. ` Since these regions
are non-overlapping, they are typically stored in either a sorted linked-list or a self-balancing binary search tree
(AVL or Red-Black).

Having a sorted tree allows for O(log(N)) queries for when searching for an address within the list of allocated
regions. This improves worst-case time complexity when there are many allocated regions and a page fault occurs.

## Heap Allocation (Byte Granular)

There are an extremely large number of heap allocation strategies available. In general, acheiving the best performance
requires choosing an allocation strategy suitable to the particular application.

For the Iris kernel, it should be rare that dynamically sized allocations are necessary. Most objects should have a size
known at compile time. Therefore, a simple free-list per object-size approach will be extremely efficent.

# Memory Management

Memory management goes beyond simply allocated memory, and pertains to the handling of page faults, COW mappings, shared
memory mappings, and disk caching.

## Physical Memory Tracking

All aspects of memory management operate on page granularity. For most memory management tasks, the kernel needs to be
able to store information about each physical page in the system. Each page requires a reference count, which counts how
many uses the page has. This comes into play because processes can share memory, either through an explicit API or
implicitly through COW mappings. Another task which requires per-page information is the file system page cache, which
needs to maintain a LRU cache of pages, so that the system can free up memory when needed. Additionally, when freeing an
address space, there needs to be some way to release all the physical memory it used.

Given that a page level tracking is required, it is important to consider how and where these page structures are
allocated. One natural approach is to lazily allocate a page structure whenever one is needed. When a page's reference
count reaches zero, this structure will not be returned to the allocator, but instead be kept in a free list. This issue
with this approach is that it requires storing the physical address of the page in the page structure, and also requires
allocating in certain cases.

A more space efficent approach is to store all physical page structures in continuous memory. Then, a page structure's
corresponding physical address can be calculated directly from its virtual address. In addition, we can easily go from a
physical address directly to a page structure, which may be useful.

### Page Structure Boostrap

One problem with the latter approach is that all pages need to be allocated upfront. Given the total number of physical
pages on the system, the kernel will be able to calculate how many pages it needs to reserve. After reserving the
physical pages, the kernel must also reserve virtual address space to store each page structure.

## Solving the Boostrapping Problem

The core issue is that the kernel primitives needed to manage memory require that a proper kernel address space is
setup. However, to modify the page tables, the kernel uses the continuous array of physical page structures as well as
the physical identity map.

The solution is to use a 2 stage bootstrapping process. In the first stage, the kernel cannot access the physical page
structures, and must use temporary page mappings to modify the page tables. Note that for now, the kernel relies on
Limine's HHDM feature to avoid having to setup temporary page mappings, although this dependency must be broken to
support other boot protocols. Once the address space is properly configured, the Iris kernel retroactively initializes
the page structures to reflect the currently mapped address space. From then on, the kernel can use the regular page
mapping routines.

### Allocating the Physical Page Structures

The first physical memory allocated by the kernel is the physical page structures. The kernel allocates them in a
physically contiguous region of memory. Since this is the first allocation, there cannot be any fragmentation, and so
this allocation should succeed. The hope behind this approach is that the kernel will be able to use huge pages to
create the mappings, which decreases the memory overhead. However, this also requires the physical pages be properly
aligned, so this is not currently done.

These physical pages are mapped into virtual memory at a fixed location, which is one PML4 entry (on x86_64), after the
physical identity map. This results in a kernel address space which looks like this (on x86_64):

```
+--------------------------+
| Physical Identity Map    |  0xFFFF800000000000 [of size equal to the physical memory available]
+--------------------------+
| Physical Structure Pages |  0xFFFF808000000000 [of size proportional to the physical memory available]
+--------------------------+
|           ...            |
+--------------------------+
| Kernel Code + Data       |  0xFFFFFFFF80000000 [of size determined by the kernel executable]
+--------------------------+
| Kernel Heap              |  0xFFFFFFFF80xxxxxx [of variable size, immediately following the kernel executable]
+--------------------------+
| Kernel Dynamic Regions   |  0xFFFFFFFF8xxxxxxx [dynamically allocated regions by the kernel]
| ...                      |
+--------------------------+
```

Currently, there is an extremely large gap between the physical structure pages and the kernel code. This is because the
Limine puts the identity map at the half-way point of the address space. This layout can be changed when dropping the
direct dependency on the Limine HHDM, which will give userspace much more available memory.

## Memory Region Backing Objects

A naive scheme for memory management assumes a 1:1 relationship between pages and address spaces. However, this is not
extensible, since it makes supporting necessary features, such as shared memory, copy-on-write memory, memory mapped, a
shared zero-page, and lazy-page allocation, impossible.

For this reason, the Iris kernel assigns each virtual region a single "backing" object, which is the single true owner
of a set of related physical pages.

### Implementing Shared Memory

Using this scheme, implementing shared memory is trivial. Two separate regions will simply point to the same reference
counted backing object. The reference counting mechanism ensures that the backing object will always be kept alive.

### Implementing Memory-Mapped Files

Memory mapped files likewise will simply use a backing object. Furthermore, this backing object will be the single
source of truth within the file system, so these objects will serve as the page cache. Even when performing normal reads
into the kernel, the memory will first be fetched into the inode's backing object, so that the values will be
automatically cached for future use. Private mappings can be created by creating a COW backing object based on the
shared backing object.

### Implementing COW Memory and the Shared Zero-Page

COW memory relies additionally on the physical page structure mechanism. Each page present in an address space has an
associated reference-count, and will not be released until this reference goes to zero. However, pages are still "owned"
by exactly one backing object. So, when creating a COW region, a new backing object is created that owns 0 pages. When
creating this mapping, the page tables are modified to prevent writing to that memory region, but the old backing
object's physical pages are used to setup the mapping. When a COW fault occurs, the reference-count can be inspected,
and if it is a 1, the object can simply take ownership of it. Otherwise, a new page is needed. The shared-zero page
simply won't be owned by an object, and so will never be freed.

There are actually 2 possible modes when creating a COW mapping. This reflects the difference between fork() and
MAP_PRIVATE. With fork(), the mapping must be isolated from the parent, so the parent's backing object must also give up
ownership of the physical pages. As a consequence, this type of mapping also requires inspecting the parent's page
tables, to copy over any previous COW mappings (for instance, from a prior fork() call). With MAP_PRIVATE, the parent's
backing object can keep ownership of the physical pages, and only the child region needs to be mapped copy-on-write.
This means that MAP_PRIVATE regions can be modified by anyone with a MAP_SHARED region to the same file, before the
child process has a chance to modify it. This seems horribly confusing, but is the sematnics of most unix systems,
including Linux.

### Implementing Backing Objects

Backing objects will primarily be represented using an intrusive RB tree of physical pages, keyed by relative page
number. This means the MM objects can be tracked without allocation, achieve O(log N) lookup, and trivially support
sparse storage. This means a MM object backing a 2 TiB disk drive uses effectively 0 extra space. Constant time lookup
is possible if a dynamic vector is used, but this requires allocations and is far too wasteful for backing files.

#### Potential Limitations

The only issue with the intrusive approach is that physical pages must be owned by only a single MM object. This doesn't
prevent implementing COW semantics, but it does prevent sharing a cache between the disk driver and file system driver,
when there is a 1:1 mapping between file system block and disk block. Consider the ext2 file system, which normally
breaks the partition into 4 KiB blocks. Each of these blocks is a single page, and corresponds exactly to a page cache
entry a block device will have. It would be nice to share these cache entries, but it is not possible to do so under
this scheme.

However, this isn't really a problem. Any "normal" file system will ensure that multiple files won't map to the same
blocks, and since all IO will go through the file system driver, there is really no need for the disk cache to do
anything.

But things a probably different when considering modern filesystems with direct support for COW semantics. For example,
on btrfs, files can clone extents, which means that files can share the same underlying data. Under this model, the
naive view proposed breaks down. On the other hand, things can be saved by considering extents to be the underlying
backing objects, and adding some higher-order backing object which combines multiples extents into a single backing
object. This can be investigated further sometime if the need truely arises.

## TLB Management

The Translation Lookaside Buffer (TLB) is a cache of virtual to physical address mappings. The processor uses this cache
to speed up page address translation. Unfortunately, the TLB is extremely problematic when supporting multiple
concurrent processors. In order to preserve coherence, individual TLB entries may to be removed from all processor's
caches. On x86_64, this procedure is managed in software.

### TLB Shootdown

In order to force other processor to invalidate their TLB entries, the kernel must send an IPI (Inter-Processor
Interrupt) to said processors. If not careful, this can easily lead to deadlock. For example, if 2 processors are
modifying different address spaces, they may need to invalidate each other's TLB entries. If each processor has
interrupts disabled, they will never receive the other's IPI, leading to deadlock.

As a direct consequence, it is unsafe to modify the page tables of an address space while interrupts are disabled. This
implies on page faults and system calls, interrupts must be enabled before executing. Additionally, allocations cannot
be made in IRQ context, as this may lead to deadlock. Although these concerns may seem like a significant limitation,
this is ultimately acceptable. On Linux, allocations can occur in IRQ context, but only using GFP_ATOMIC, which is
liable to fail. If needed, the Iris kernel can provide a similar API, although IRQ handlers should really be
preallocating any memory they need. Page faults and system calls are already expected to be preemtible, so this is not a
concern.

The actual IPI mechanism also cannot allocate memory, so IPI messages must be stored in a preallocated object pool. Each
processor will have a fixed-sized ring buffer which stores pointers to pending IPI messages. This way, when broadcasting
an IPI, only a single message is needed. The object pool and each processor's queue are protected by separate spinlocks.
