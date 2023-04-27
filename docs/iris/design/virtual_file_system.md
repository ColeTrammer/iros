# Virtual File System

## What is it?

The virtual file system is a kernel level abstraction which presents files and directories to userspace. The "virtual"
adjective comes from the fact that this is generic over a physical disk layout. In practice, this means applications
access files uniformly whether the underlying file system is ext2, FAT-32, btrfs, etc.

In addition to modelling the directory hiearchy, the VFS provides an interface for reading and writing files. Under the
Unix philosophy of "everything is a file", this abstraction extends not just to writing files, but accessing timers,
user input events, the GPU, networking connections, random numbers, hard drives, kernel parameters, process
introspection, and essentially any possible kernel interface which can be modelled as a read or a write.

## Orthogonal Concerns

From the above description, it is clear that the VFS is more than just an abstraction over physical file systems, but
typically is the core abstraction processes use to communicate with the kernel. Because of this, it is important to
recognize that "real" file systems have different requirements than the more abstract ones, and the kernel needs to
design for this fact.

# Userspace API

All userspace interaction with the file system is carried out through one of 2 mediums: paths and file descriptors.

## Paths

Traditionally, a path is a slash separated string, which is an indirect reference to a VFS entity. It is indirect in the
sense that path lookup is inherently fallible; another application can delete the path at any time.

In [Redox](https://www.redox-os.org/), paths have a URL like syntax, in that the can contain an optional scheme. The
schemes serve as a sort of root namespace, and each namespace is free to interpret the following data anyway it wishes.

This is an attempt at normalizing the file system, and provides a way to get rid of the wide variety of special case
file systems which are otherwise needed to provide a heterogenous APIs. For example, a socket can be opened using
'tcp:0.0.0.0' in Redox, while in a Unix system which supported opening sockets from the file system, a path would have
to be provided: '/dev/tcp/0.0.0.0'.

The URL scheme can extend to many different types of files, including physical devices (typically mounted in /dev),
process information (typically mounted in /proc), and system configuration (typically mounted in /proc/sys). One can
also imagine how this would make opening special files, like timers, shared memory, unix domain sockets, etc. possible
through a more homogenous API.

However, such an API does further conflate physical files to kernel level interfaces, and so I am unsure whether or not
it is truely a good idea.

## File Descriptors

In a modern kernel, a file descriptor is simply an opaque handle to a kernel resource. Note that this really has nothing
to do with a file on stored in a "real" file system. On Linux, a file descriptor can refer to any of: file, directory,
VFS location (O_PATH), io_uring instance, timerfd, eventfd, epoll instance, network socket, FIFO, block device, special
device, terminal, shared memory, POSIX semaphore, etc...

For backwards compatibility, a file descriptor has many associated flags and state which are essential useless for
anything other than a regular file. The most notable is the implicit file offset.

Additionally, because of system calls like dup (2), file descriptors themselves refer to a reference counted file object
in the kernel. This file object can be shared between processes (inherited across fork (2)). Thus, it is rather easy to
construct scenarios where multiple processes write to the same file at once. For a regular file, this causes
non-sequential writes as the implicit file offset changes between calls. Terminals actually go out of their way to
handle this scenario, by implementing an extremely arcane signalling mechanism (SIGTTOU and SIGTTIN), which is often
forcibly disabled (since their default disposition is terminated the process).

In the Iris kernel, it is a goal to simplifiy this interface by keeping as much state as possible outside of the kernel.
This can be done by implementing file descriptors in userspace, as some sort of front to a real kernel handle. Removing
the implicit file offset can be done by keeping track of this in userspace. Any use-case which requires the kernel to
synchronize successive reads and writes should strongly reconsider their design. Using a FIFO or socket (which has no
offset whatsoever), should be used if this is really necessary.

## Kernel File System Model

To model "real" file systems, the kernel needs virtual objects which provide a generic way to descripe the underlying
file system constructs.

### File System Composition (Mount)

In a normal unix system, there is a single file system namespace, but there any many different file systems accessible.
This is representing using a mount, which is conceptually a mapping from a path to a file system instance. Mounts can
overlay on top of real directories, and they take precedence over the underlying file system. Additionally, some systems
support mount flags and bind mounts, which can be used to mark a subtree of the file system as read-only.

### File System Instance (Super Block)

Any operational file system needs to store some form of state in order to access its root directory, and will likely
include other configurable settings. This structure is typically known as the super block, which stems from the fact
that is usually the first block of the file system (which "describes" all other blocks).

Its existance is somewhat uninteresting, given only 1 exists per file system instance, and is strictly necessary to
implement any file system.

### File System Entity (Inode)

An inode (index-node), stores the metadata about a file system entity. This includes access protections, bookkeeping
fields, and the location on disk of its content blocks (raw data). In most file systems, inodes can be referred to from
many different paths, due to both the presence of symbolic links and hard links.

The name likely comes from the fact that inode's have a unique index within a given file system. Combining this index
which a unique identifier specific to the given file system uniquely identifies a file system entity.

The inode is the work-horse of file system abstraction, since it provides a way to get to the data blocks and metadata
associated with the entity.

### File System Anchor (Tnode / dentry)

A file system anchor (known as a tree-node or directory entry), refers provides a reference to a inode, with an
assoicated name and chain of parent tnodes which represent one possible path to the inode. Tree node's are the result of
a path lookup, and are usually the correct way to store a reference to an inode. Since tree-node's have a chain of
parents, the original path used to find the inode may be reconstructed.

The split between tree-nodes and inodes is required to support different types of file system links. Tree-nodes are data
structure supporting path lookups, and as such, caching them becomes highly relevant to support fast file system access.

One key point to realize about tree-nodes is they strongly reference the underlying inode. They provide memory safety in
the prescense of external modifications. For instance, imagine a user shell session with a current working directory of
`a/b/c`. If another user were to delte directory `c`, the user's shell's current working directory remains unchanged.
Tree-node's ensure the virtual inode is not deleted until the last reference is dropped. But since the file is deleted
from disk, the user will encounter errors when attempting to `ls` in their shell, since deleted files cannot be opened.

## Path Resolution

Path resolution takes a path as input and produces a tree-node as output. This operation serves as the basis for all
filesytem APIs. It also is complex, having to handle mounts, symbolic links, and concurrent modifications to the
underlying file system.

### Algorithm

The algorithm for path resolution is relatively simple, and can be summarized as follows:

1. Start from a base tree-node (either the root, current working directory, or a provided tree-node (e.g. openat())).
2. Iterate over each path component, and:
    1. If the component is `.` or empty, continue.
    2. If the component is `..`, follow the tree-node's parent pointer.
    3. Otherwise, lock the current tree-node and perform a lookup of the component in the associated inode.
    4. If this is a symbolic link, recurse.

This algorithm also needs to handle mounts, which is done in step 2.3 by considering the mount when performing the
lookup.

### Optimization

The algorithm above is correct, but it is not very fast. The main problem is that it requires a lock to be taken for
every path component, and performing the actual lookup may require disk access. The obvious answer is to use some sort
of caching.

The tree-node cache can effectively be stored in a hash table with the key being a path component and the parent
tree-node.

## Page Cache

Although directory entry caching is crucial for high-speed path lookup, the page cache is most important mechanism for
speeding-up file system access.

The basic idea is simple: the kernel simply maintains a cache of recently read file data, and uses it to satisify
incoming read requests whenever possible. In practice, this becomes a bit more complex, because the page cache must
seamlessly integrate with disk flushing, page faults, OOM reclaimation, userspace process memory management, and the
file system driver itself.

### From the Perspective of a File System Driver

The key insight for creating a simple design is to push every file system read to page granularity and always go through
the page cache. Imagine the user requests to read 4 bytes at offset 0x1006 from a file. The kernel will model this
request instead as a page sized read at offset 0x1000, and store the returned page in the page cache. Once this read
completes, the small chunk of data requested will be copied by the kernel into the intended buffer.

Under this approach, caching disk reads is automatic, which means that when the userspace program then attempts to read
another 4 bytes from offset 0x100a, the kernel will be able to fulfill the request without even talking to the file
system driver.

The only potential downside is that data must be copied an extra time to support always saving new data to the page
cache. In the case where applications need to care (think databases), they can be allowed to bypass the page cache given
they pre-allocate their own page-aligned buffers with the kernel before hand. In the common case, the overhead of
reading to disk will be significantly larger than the cost of performing a memory copy, and upside of caching is simply
to large. Additionally, applications can memory map a file directly which will grant them access to the memory stored in
the page cache directly.

### From the Perspective of the VFS

Files with physical on disk content will always have an assoicated virtual memory object, which contains references to
physical pages which store the file's content. This data should be lazily populated and reclaimable by the memory
management system.
