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

_mount_ - called with a `device_path` to mount the file system somewhere. It returns the root tnode of the file system.

_determine_fsid_ - called with a `block_device` in order to determine its file sytem id.

_list_ - inline linked list field to store all registered file systems.

_id_table_ - array of all partition ids (MBR/GPT) that this file system cares about.

_id_count_ - count of elements in the above array.

## super block

This object represents a large scale overview of the mounted file system. For any given mounted file system, it must have a super_block object.

### struct super_block

_fsid_ - field contiaining the dev_t number of the super block.

_root_ - field containing a pointer to the root inode of the mounted file system

_op_ - field containing a pointer to the `super_block_operations` for the object

_block_size_ - field containing the block size of the file system

_num_blocks_ - number of blocks in file system

_free_blocks_ - number of free blocks in file system

_available_blocks_ - number of available blocks in file system

_num_inodes_ - number of inodes in file system

_free_inodes_ - number of free inodes in file system

_available_inodes_ - number of available inodes in file system

_dirty_super_blocks_ - inline list of all dirty super blocks.

_device_ - field containing the backing device of the file system, if it has one.

_dirty_ - boolean flag used to know when to sync this structure to disk.

_super_block_lock_ - field that locks this object; it should be locked if any changes to the object are being made

_private_data_ - field for arbitrary data to be passed to anything that has access
to the `super_block`, which should be most file system objects.

### struct super_block_operations

This object has the operations that can currently be done on a super_block

_rename_ - function that renames an existing tnode to a new name under a new
parent. Note that the actually renaming of the tnode is done by the vfs and
this function should only modify disk to reflect that.

_sync_ - function that syncs the super block to disk.

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

_f_op_ - field that points to operations that can be done to a file.

_flags_ - field that contains the inode->flags of a file's corresponding inode. This
probably should not be used, and instead the information should be gathered by
getting at the inode.

_open_flags_ - field storing the falgs a file was opened with. This is currently ignored, but should be taken into consideration. `O_APPEND` is pretty important...

_abilitites_ - field is pretty much a parsed version of open_flags, concerning `O_WRONLY`, `O_RDONLY`, and `O_RDWR`. This should be removed now that all of `open_flags` is stored.

_ref_count_ - field is the file's reference count.

_lock_ - mutex lock of file

_tnode_ - tnode of the open file (NULL for certain socket and pipe files)

_inode_ - inode of open file (NULL for certain socket files)

_private_data_ - can be used to store arbitrary data

### struct file_operations

_close_ - can be overriden (not NULL), if the file wants to do something **_when dropped_**. This is only called when close causes the file count to drop to 0, not when `fs_close` is called. So far, no file system needs to actually override close, but the name of the function
should be changed to avoid eventual confusion.

_read_ - read from the file; the file system is pretty much useless without this (unless you can mmap the file)

_write_ - write to the file; this should be NULL in read only file systems (and for directories).

## tnode

Object to managed named inode or tree-nodes, as well as store them in a list for directories.

### struct tnode

_name_ - name of tnode

_inode_ - pointer to the tnode's inode. This must be non-null

_parent_ - parent tnode

_ref_count_ - reference count

## inode

Object that represents a 'file system object' to the vfs.

### struct inode

_mode_ - mode of an inode (stores the permissions and type)

_flags_ - vfs readable type of the inode (could be completely removed)

_i_op_ - operations to be done on an inode

_super_block_ - pointer to the `super_block` an inode resides in

_socket_ - pointer to possibly bound AF_UNIX socket

_uid_ - user id of inode

_gid_ - group id of inode

_access_time_ - last recent access time of inode

_modify_time_ - time last modified

_change_time_ - time last changed

_fsid_ - file system id of inode

_device_id_ - device id of inode (as in struct stat::st_rdev)

_size_ - size of inode

_index_ - unique identifier within the file system

_mounts_ - lists of mounts points under this inode, if it is a directory

_readable_ - readable flag for select

_writeable_ - writeable flag for select

_exceptional_activity_ - exceptional flag for select

_dirty_ - dirty flag for asynchronous disk sync

_ref_count_ - inode reference count

_open_file_count_ - count of the number of times this inode is currently opened

_dirent_cache_ - directory cache for directory inodes

_vm_object_ - backing vm object for mmap()

_dirty_inodes_ - inline linked list node to keep track of dirty inodes

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

_mknod_ - make device node

_unlink_ - unlink an inode from the file system

_rmdir_ - remove a directory from a file system

_chmod_ - change modifiers for an inode

_chown_ - change owner of inode

_mmap_ - mmap in a given inode

_symlink_ - make symlink

_link_ - make hard link

_read_all_ - read inode's contents

_utimes_ - read timestamps

_on_inode_destruction_ - callback for when an inode's ref count reaches zero. This usually means that it was unlinked (but not necessarily).
All open files pointing to it have been closed at this point.

_read_ - read directly from inode

_truncate_ - truncate inode

_sync_ - sync inode to disk
