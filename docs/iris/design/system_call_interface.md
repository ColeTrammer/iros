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
