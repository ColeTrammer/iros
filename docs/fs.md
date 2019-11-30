# fs

## virtual file system *(vfs)*
The vfs has all the kernel methods that implement file system related
system calls. For example, fs_open corresponds to open(2), fs_close corresponds
to close(2). 

Unrelated code to system calls is for general purpose use:

### fs_clone
This method clones an existing file. Semantically, this creates a cloned file
that points to a different address, but contains the same information as the
cloned file.

### fs_dup
This method duplicates an existing file. This simply bumps the ref count on
the file object.

### fs_bind_socket_to_inode
This is used by AF_UNIX sockets to bind themselves to a particular inode. This
is pretty much just bookkeeping information for when another program tries to
connect to a specific file system path.

### load_fs
This method registers a file system object with the vfs, allowing it to be
mounted and understood.

### get_full_path
This method allocates a path on the kernel heap, that is a combination of
the cwd and the relative_path.

### get_tnode_path
This method gets the canonical path of a given tnode in the file system.

### drop_inode_reference and drop_inode_reference_unlocked
These functions decrement the inode reference count and destoy the inode if
necessary. The unlocked suffix means the vfs will not try to lock the inode
before decrementing the reference count. This should only be used when the
inode is already locked, so this may be a terrible name.

## file_system
This object represents a given file system, independent on its mounts.

### struct file_sytem

*name* - name of file system

*flags* - currently unused, but maybe someday...

*mount* - called with a `device_path` to mount the file system somewhere. It returns
the root tnode of the file system.

*super_block* - should be filled in when a `file_system` is mounted. This implies
the vfs will clone this object for each mount of it.

*next* - linked list pointer used to store these objects

## super block
This object represents a large scale overview of the mounted file system. For
any given mounted file system, it must have a super_block object.

### struct super_block

*device* - field contiaining the device this is mounted on, or an arbitrary
identifier if this file system is not disk backed.

*root* - field containing a pointer to the root tnode of the mounted file system

*op* - field containing a pointer to the `super_block_operations` for the object

*block_size* - field containing the block size of the file system

*dev_file* - field containing a file referring to the backing device of the file
system, if it has one.

*super_block_lock* - field that locks this object; it should be locked if any
changes to the object are being made

*private_data* - field for arbitrary data to be passed to anything that has access
to the `super_block`, which should be most file system objects.

### struct super_block_operations
This object has the operations that can currently be done on a super_block

*rename* - function that renames an existing tnode to a new name under a new
parent. Note that the actually renaming of the tnode is done by the vfs and
this function should only modify disk to reflect that.

## mount
This object represents a mount point in the vfs.

### struct mount

*device_path* - path of backing device

*name* - name of the mount point (like "proc" for the proc file system)

*fs* - pointer to the file system object of this mount point

*super_block* - pointer to the `super_block` of this mount

*next* - linked list storage field

## file
This object represents an open file in the kernel. Should be obtained via `fs_open`.

### struct file

*inode_identifier* - field that stores the inode->index that corresponds to this file.

*length* - field that stores the length of a file's inode. This field should never be
used, since inode->size can change without the file knowing.

*start* - field that stores the start of the file within a backing device. Again this
should never be used and is a remnant of the early initrd implementation.

*position* - field that corresponds to the file's lseek(2) based position.

*f_op* - field that points to operations that can be done to a file.

*flags* - field that contains the inode->flags of a file's corresponding inode. This
probably should not be used, and instead the information should be gathered by
getting at the inode.

*fd_flags* - field is completely broken, but stores whether or not a file is
`FD_CLOEXEC`. Unfortunately, this field cannot be shared by duplicated files and thus
needs to be stored differently (only this field is not the same accross files duplicated
using dup(2) of dup2(2)).

*open_flags* - field storing the falgs a file was opened with. This is currently ignored, 
but should be taken into consideration. `O_APPEND` is pretty important...

*abilitites* - field is pretty much a parsed version of open_flags, concerning `O_WRONLY`, `O_RDONLY`, and `O_RDWR`. This should be removed now that all of `open_flags`
is stored.

*ref_count* - field is the file's reference count.

*lock* - used to lock the file

*devuce* - backing device of the file

*private_data* - can be used to store arbitrary data

### struct file_operations

*close* - can be overriden (not NULL), if the file wants to do something ***when dropped***.
This is only called when close causes the file count to drop to 0, not when `fs_close` is called. So far, no file system needs to actually override close, but the name of the function
should be changed to avoid eventual confusion.

*read* - read from the file; the file system is pretty much useless without this (unless you
can mmap the file)

*write* - write to the file; this should be NULL in read only file systems (and for directories).

*clone* - used to have a hook when a file is cloned. Since `fs_clone` is never called in
the kernel, this method also won't be called. Nevertheless, pipes care about this so
it remains, since `fs_clone` should be used eventually.

## tnode
Object to managed named inode or tree-nodes, as well as store them in a list for directories.

### struct tnode

*name* - name of tnode

*inode* - pointer to the tnode's inode. This must be non-null

### struct tnode_list

*tnode* - pointer to the tnode of this list

*next* - pointer to next list entry

### add_tnode
Add a tnode to a given list

### remove_tnode
Remove a tnode from a given list

### find_tnode
Find a tnode by name in a given list

### find_tnode_inode
Find a tnode by a given inode in a given list

### find_tnode_index
Find a tnode by index in a given list

### get_tnode_list_length
Get length of tnode list

### free_tnode_list_and_tnodes
Destroy a given tnode list and all the the tnodes it contains.

## inode_store
Stores inode by index and device for lookup by files. This is implemented as
a hash map of hash maps.

### struct inode_store

*device* - deivce of particular store

*map* - map of inodes within a particular `device`

### fs_inode_get
Takes in a dev_t and ino_t, gives an inode back. This is how files get their backing
inode.

### fs_inode_put
Add inode to be remembered.

### fs_inode_set
Set an inode to a new one.

### fs_inode_del
Remove an inode from the maps.

### fs_inode_free_store
Remove a store object (presumably the file system was unmounted)

### fs_inode_create_store
Create a store object (presumably the file system was mounted)

## inode
Object that represents a 'file system object' to the vfs.

### struct inode

*mode* - mode of an inode (stores the permissions and type)

*flags* - vfs readable type of the inode (could be completely removed)

*i_op* - operations to be done on an inode

*super_block* - pointer to the `super_block` an inode resides in

*socket_id* - 0 if the inode is not bound to a socket, otherwise the
socket id of a AF_UNIX bound socket.

*device* - device id of file system

*size* - size of inode

*index* - unique identifier within the file system

*tnode_list* - lists of tnodes if this is a directory

*mounts* - lists of mounts points under this inode, if it is a directory

*parent* - parent tnode of this inode, points to its own tnode if root (this
will be adjusted automatically on root inodes not mounted as the root).

*ref_count* - inode reference count

*lock* - inode lock (for everything)

*private_data* - used to store arbitrary data

### struct inode_operations

*create* - make an inode at a parent tnode with given name and mode

*lookup* - used by the vfs to find a given name within a directory. If
called with the argument NULL (for name), then the file system is supposed
to fill in the inode's `tnode_list`.

*open* - open an inode

*stat* - stat an inode

*ioctl* - do an ioctl on this inode

*mkdir* - make a directory for a given tnode with a given name and mode

*unlink* - unlink an inode from the file system

*rmdir* - remove a directory from a file system

*chmod* - change modifiers for an inode

*mmap* - mmap in a given inode

*on_inode_destruction* - callback for when an inode's ref count reaches zero.
This usually means that it was unlinked (but not necessarily). 
All open files pointing to it have been closed at this point.