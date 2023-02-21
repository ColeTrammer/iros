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
