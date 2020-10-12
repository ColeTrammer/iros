# fs

## virtual file system _(vfs)_

The vfs has all the kernel methods that implement file system related
system calls. For example, fs_open corresponds to open(2), fs_close corresponds
to close(2).

Unrelated code to system calls is for general purpose use:

### fs_dup

This method duplicates an existing file. This simply bumps the ref count on
the file object.

### fs_bind_socket_to_inode

This is used by AF_UNIX sockets to bind themselves to a particular inode. This
is pretty much just bookkeeping information for when another program tries to
connect to a specific file system path.

### register_fs

This method registers a file system object with the vfs, allowing it to be
mounted and understood.

### get_full_path

This method allocates a path on the kernel heap, that is a combination of
the cwd and the relative_path.

### get_tnode_path

This method gets the canonical path of a given tnode in the file system.

### drop_inode_reference

This function decrements the inode reference count and destoy the inode if
necessary.

## file_system

This object represents a given file system, independent on its mounts.

### struct file_sytem

_name_ - name of file system

_flags_ - currently unused, but maybe someday...

_mount_ - called with a `device_path` to mount the file system somewhere. It returns
the root tnode of the file system.

_super_block_ - should be filled in when a `file_system` is mounted. This implies
the vfs will clone this object for each mount of it.

_next_ - linked list pointer used to store these objects

## super block

This object represents a large scale overview of the mounted file system. For
any given mounted file system, it must have a super_block object.

### struct super_block

_device_ - field contiaining the device this is mounted on, or an arbitrary
identifier if this file system is not disk backed.

_root_ - field containing a pointer to the root tnode of the mounted file system

_op_ - field containing a pointer to the `super_block_operations` for the object

_block_size_ - field containing the block size of the file system

_dev_file_ - field containing a file referring to the backing device of the file
system, if it has one.

_super_block_lock_ - field that locks this object; it should be locked if any
changes to the object are being made

_private_data_ - field for arbitrary data to be passed to anything that has access
to the `super_block`, which should be most file system objects.

### struct super_block_operations

This object has the operations that can currently be done on a super_block

_rename_ - function that renames an existing tnode to a new name under a new
parent. Note that the actually renaming of the tnode is done by the vfs and
this function should only modify disk to reflect that.

## mount

This object represents a mount point in the vfs.

### struct mount

_device_path_ - path of backing device

_name_ - name of the mount point (like "proc" for the proc file system)

_fs_ - pointer to the file system object of this mount point

_super_block_ - pointer to the `super_block` of this mount

_next_ - linked list storage field

## file

This object represents an open file in the kernel. Should be obtained via `fs_open`.

### struct file

_inode_identifier_ - field that stores the inode->index that corresponds to this file.

_position_ - field that corresponds to the file's lseek(2) based position.

_f_op_ - field that points to operations that can be done to a file.

_flags_ - field that contains the inode->flags of a file's corresponding inode. This
probably should not be used, and instead the information should be gathered by
getting at the inode.

_fd_flags_ - field is completely broken, but stores whether or not a file is
`FD_CLOEXEC`. Unfortunately, this field cannot be shared by duplicated files and thus
needs to be stored differently (only this field is not the same accross files duplicated
using dup(2) of dup2(2)).

_open_flags_ - field storing the falgs a file was opened with. This is currently ignored,
but should be taken into consideration. `O_APPEND` is pretty important...

_abilitites_ - field is pretty much a parsed version of open_flags, concerning `O_WRONLY`, `O_RDONLY`, and `O_RDWR`. This should be removed now that all of `open_flags`
is stored.

_ref_count_ - field is the file's reference count.

_device_ - backing device of the file

_private_data_ - can be used to store arbitrary data

### struct file_operations

_close_ - can be overriden (not NULL), if the file wants to do something **_when dropped_**.
This is only called when close causes the file count to drop to 0, not when `fs_close` is called. So far, no file system needs to actually override close, but the name of the function
should be changed to avoid eventual confusion.

_read_ - read from the file; the file system is pretty much useless without this (unless you
can mmap the file)

_write_ - write to the file; this should be NULL in read only file systems (and for directories).

_clone_ - used to have a hook when a file is cloned. Since `fs_clone` is never called in
the kernel, this method also won't be called. Nevertheless, pipes care about this so
it remains, since `fs_clone` should be used eventually.

## tnode

Object to managed named inode or tree-nodes, as well as store them in a list for directories.

### struct tnode

_name_ - name of tnode

_inode_ - pointer to the tnode's inode. This must be non-null

### struct tnode_list

_tnode_ - pointer to the tnode of this list

_next_ - pointer to next list entry

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

_device_ - deivce of particular store

_map_ - map of inodes within a particular `device`

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

_mode_ - mode of an inode (stores the permissions and type)

_flags_ - vfs readable type of the inode (could be completely removed)

_i_op_ - operations to be done on an inode

_super_block_ - pointer to the `super_block` an inode resides in

_socket_id_ - 0 if the inode is not bound to a socket, otherwise the
socket id of a AF_UNIX bound socket.

_device_ - device id of file system

_size_ - size of inode

_index_ - unique identifier within the file system

_tnode_list_ - lists of tnodes if this is a directory

_mounts_ - lists of mounts points under this inode, if it is a directory

_parent_ - parent tnode of this inode, points to its own tnode if root (this
will be adjusted automatically on root inodes not mounted as the root).

_ref_count_ - inode reference count

_lock_ - inode lock (for everything)

_private_data_ - used to store arbitrary data

### struct inode_operations

_create_ - make an inode at a parent tnode with given name and mode

_lookup_ - used by the vfs to find a given name within a directory. If
called with the argument NULL (for name), then the file system is supposed
to fill in the inode's `tnode_list`.

_open_ - open an inode

_stat_ - stat an inode

_ioctl_ - do an ioctl on this inode

_mkdir_ - make a directory for a given tnode with a given name and mode

_unlink_ - unlink an inode from the file system

_rmdir_ - remove a directory from a file system

_chmod_ - change modifiers for an inode

_mmap_ - mmap in a given inode

_on_inode_destruction_ - callback for when an inode's ref count reaches zero.
This usually means that it was unlinked (but not necessarily).
All open files pointing to it have been closed at this point.
