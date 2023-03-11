# Memory Allocation

In general, memory allocation consists of 3 components:

-   Physical Page Frame Allocator
-   Virtual Memory Allocator (Page Granularity)
-   Heap Management (Byte Granularity)

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
physical pages, the kernel must also reserve virtual address space to store each page structure. Additionally, physical
backing memory most likely will be needed as well.
