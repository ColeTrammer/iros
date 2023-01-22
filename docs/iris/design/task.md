# Task Model in the Iris Kernel

## What is a task?

In today's operating systems, the is a notion of processes and a notion of threads. Processes have isolated address spaces and resources, and each can have many threads.

A thread represent an executable instruction stream, and is the concept relevant to the scheduler. At all times, 1 task should be executing on 1 CPU core, unless the CPU itself is idle.

## How is a process represented?

In Linux, there is no kernel level structure representing a process. Instead, threads (called tasks) are grouped together by sharing attributes (like the memory region, PID, etc.). This has some consequences, because attributes which are not shared between tasks on Linux (euid, egid) but are shared between threads according to POSIX, must have their sharing be emulated in userspace.

In general, the whole process structure is essentially redundant, given almost all kernel operations should operate on threads and not processes. This includes signalling, blocking, scheduling, and tracing. Memory allocation can operate on a dedicated object which is shared between threads, as can process attributes (pid, pgid, sid, uid) if they are to be supported directly in the kernel. File system operations are more interesting, since they can be made more efficent by not allowing file sharing between threads (think io_uring registered files), but are expected to be shared by POSIX. This can be solved by introducing a kernel object representing a file table. Then, the runtime library will create a shared file table for each process, but dius::IoContext can have its own private file table, which cannot be accessed by other threads.

Therefore, at least initially, there will be no notion of a process, only tasks. This contrasts the normal OSDEV approach, which only represents processes, not threads, and thus requires large design changes to support multi-threading.

## How is a task represented?

A task must all the information required to perform a context switch. This means it must store all relevant registers, which must be loaded when it comes to execute the task. In addition, some state variable is used to determine if a task is (running or blocked), although this notion is mostly useful for scheduling.

A task also contains ISA level information necessary to run programs on a given architecture. On x86_64, this includes the base address of the %gs register, which stores the userspace thread-control-block which serves as the basis for the userspace threading implementation.

Tasks also require their own stack, which is a dedicated virtual memory region in the kernel which is used to service any interrupts or system calls which occur while the task is executing. Note this is not shared between processes, and so it is not part of the process virtual memory map.

Lastly, a task has some shared process state which includes the process virtual memory map.