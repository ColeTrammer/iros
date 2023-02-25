# System Call Interface

## Traditional Model for System Calls

On x86_64, there are 2 mechanism for performing system calls. The first option, which is
essentially for backwards compatibility, is to programmatically generate an interrupt into
the kernel.

The modern approach is using amd64's dedicated `syscall` instruction, which transfers control
directly to the kernel.

Both these mechanisms effectively context switch into the kernel. As such, the kernel is free
to reschedule the calling task at this point, and can also block the task if necessary.

## Asynchronous Model for System Calls

Under Linux's io_uring mechanism, kernel system calls can effectively be performed without
even context switching into the kernel. This is done by having userspace and the kernel communicate
through ring buffers. If the kernel has a dedicating polling thread, it never needs to be woken up
by the calling task.

However, in most cases, the userspace code needs to call io_uring_enter(), which gives the kernel a
change to process the queued system calls. This can still be efficent than performing synchronous system
calls, even for trivial operations, since the system calls can be batched together (which reduces the number
of context switches required).

## Asynchronous First System Calls

In the Iris kernel, system calls must be asynchronous first, in order to allow for high throughput scaling.
However, most userspace code uses synchronous APIs, and even most asynchronous code uses readyness based APIs
(think poll (2) or select (2)), which the Iris kernel will use a completion based API. As a consequence, care
must be taken to ensure the approach chosen is capable of emulating these other APIs.

The idea is that internally, system calls which should be asynchronous will be modelled as asychronous. There
will be a io_uring like mechanism which allows userspace to submit multiple system calls at once. However, there
will also be a normal system call ABI, which implicitly blocks for the asynchronous underlying API to complete.
This will be used to emulate synchronous APIs for compatibility.

### ABI

It is typical to support passing 6 arguments to system calls, so the kernel system call queue will be sized to
fit exactly 6 arguments. This will ensure that from an API perspective, system calls using the queue will be
the same as calling the explicitly synchronous version.

### Note on Null-Terminated Strings

The POSIX C API passes paths as null-terminated C strings. This prevents applications which support view types, like C++
and Rust, from easily using these APIs. In fact, the view types usually have to be copied simply to add a zero to the
end of the buffer. Null-terminated strings are also far more annoying to deal with in the kernel.

As a consequence, the Iris kernel will not support null-terminated strings. Instead, system call arguments will be
passe as a pointer + size pair. When emulating POSIX APIs, the system call wrapper must call `strlen` before entering
the kernel.

### Emulating Readyness APIs

With this approach, modelling readiness can be done by making an asynchronous query, which effectively completes
when the resource is ready. Userspace can simply submit one of these requests when the caller a blocking function,
and then it will wait on for completion.

It is also possible to emulate readyness purely in userspace, by giving file descriptors a private buffer. Then, a
readiness query would complete when either data arrives in the better or there is now extra space in the buffer. This
may prove difficult to achieve, especially when considering non-blocking reads.

### Emulating Traditional Non-blocking IO

Emulating non-blocking read requests is the tricky part. Imagine trying to read data from a internet socket. Using a
pure completion API, the only way to know if data is ready is to submit a read request and wait for it to complete.
However, we want to return immediately if there is no data. This can of course be done by having a special mode when
reading data which never blocks, but that would be non-general and most likely make code a lot more complicated.

One idea is to use some sort of time out mechanism, which will try to cancel the IO request after a set amount of time.
If the timeout is 0, the operation must complete immediately. However, depending on how this is implemented, it may
require spinning up a background worker only to immediately cancel it.

## Task Creation

Traditionally, unix systems use a combination of `fork` and `exec` to implement task creation. This approach is not
maximally efficent, because `fork` requires creating a COW mapping of the process's address space. What's worse is that in
most cases, this the created process's address space is immediately thrown away by a call to `exec`. As such, the Iris
kernel will not implement task creation using this API.

Instead, the Iris kernel will support a rich set of system calls for task creation. For instance, to create a new
process from some binary, a userspace program would be expected to submit the following multiple system calls:

1. create_task (which will create a new kernel task, with no attributes set)
1. load_executable (which will initialize the tasks address space)
1. share_files (which will give the new task access to stdin, stdout, and stderr)
1. start_task (which will schedule the task now that it has been fully initialized)

One point to note is that because the underlying system call API will allow for queueing multiple system calls at once,
this will not require performing more than one context switch to create the new task. Additionally, a system call like
share_files can be generally useful outside of task creation, and maybe can be used with running tasks as well. More
generally, a system call in POSIX like `setpgid` will be implemented in the Iris kernel as `set_task_pgid`, and take the
task whose process group id should be set as the first argument. In this way, the `set_task_pgid` can be used both to
set another tasks process group id, as well as its own. This ensures that this task creation mechanism won't require
duplicating a large number of system calls.

### Emulating POSIX

It's clear that the above mechanism can support `posix_spawn`, given that each system call involved could map directly
to an "action" in terms of `posix_spawn`.

More interestingly is how `fork` can be emulated using this mechanism. All that is needed is for the kernel to provide
a dedicated system call, such as `create_cow_mapping`, which will give the newly created task a copy of another address
space. In combination with some additional system calls for directly modifying a task's state, such as
`set_instruction_pointer` and `set_cpu_register`, the exact API of fork() can be emulated. Of course, anything shared
accross `fork` would require some system call which duplicates the resource to the newly created task, but for most
attributes this will be necessary anyway, without even considering supporting `fork`.

A `fork` emulating with slightly different semantics can likely be used to implement `vfork` as well, although POSIX
applications which use `vfork` should probably be written to use `posix_spawn` instead whenever possible, which will
ensure that their process creation strategy will be optimized for the target platform.

## Platform Specific ABI

### x86_64

For synchronous system calls, the calling convention will be identical to Linux's. In particular, the arguments will be
passed in the following registers. Note that we cannot use the regular SYS-V calling convention because the `rcx` and
`r11` registers is clobbered when using the `syscall` instruction.

| ABI Component      | CPU Register | Clobbered                                                   |
| ------------------ | ------------ | ----------------------------------------------------------- |
| System Call Number | `rax`        | Yes                                                         |
| Argument 1         | `rdi`        |                                                             |
| Argument 2         | `rsi`        |                                                             |
| Argument 3         | `rdx`        | Yes                                                         |
| Argument 4         | `r10`        |                                                             |
| Argument 5         | `r8`         |                                                             |
| Argument 6         | `r9`         |                                                             |
| Saved `rip`        | `rcx`        | Yes (when using `syscall` instruction)                      |
| Saved `rflags`     | `r11`        | Yes (when using `syscall` instruction)                      |
| Flags Register     | `rflags`     | Yes (kernel may mutate userspace `rflags` before returning) |
| Success Return     | `rax`        |                                                             |
| Error Return       | `rdx`        |                                                             |

Note that this does differ from the linux system call abi in that the return value is sent through 2 registers. On
Linux, error conditions are represented using negative return values of `rax`. Instead, the Iris kernel sets `rdx` to 0
on success, and `rdx` will be non-zero when an error occurs. This extension makes checking for error conditions simpler
for all system calls, and ensures the return value can use the full range of 64 bit values. It is imagined that this
distinction can also enable APIs to report partial success, for instance, in scenarios where a disk read is only able to
complete parts of the request.

An alternative ABI for indicating an error would be to have to kernel directly set one of the bits in `flags`, and have
the caller branch on that. This has the benefit of not clobbering the `rdx` register but is far more challenging to use
with inline assembly.

Additionally, the Iris kernel will support the `syscall` instruction in the future, but currently implements system
calls using the 128th interrupt. Support for this should be removed once `syscall` is added, because Iros does not need
to worry about backwards compatibility.
