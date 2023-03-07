# Synchronization Primitives

## Purpose

Many functions within the kernel can be called from different context simultaneously. This can happen either because
there are multiple processors running, because the kernel was pre-empted while executing the function in question for
another task, or because a HW interrupt fired during the execution of said function.

To enable these scenarios to work properly, the kernel must have various synchronization primitives which ensure that
scenarios like these are handled correctly.

## Safety

Different functions require different levels of synchronization safety depending on some pre-conditions which can be
imposed on the caller. This mainly concerns what type of execution context is allowed to call the function in question.

| Execution Context | Description                                                         |
| ----------------- | ------------------------------------------------------------------- |
| HW Interrupt      | A function can safely be called inside a hardware interrupt handler |
| Normal            | A function can be safely called by any kernel level task            |

The reason for this distinction is that functions which can be called within an HW interrupt handler **MUST** disable
interrupts during their execution. If this does not occur, then a HW interrupt may fire during the function's execution,
and when the hardware interrupt attempts to call said function, a deadlock will occur.

In contrast, functions which can never be called from interrupt context _should not_ disable interrupts, to ensure the
system is responsive.

### Note on Asynchronous Execution

Assuming the Iris kernel implements a lighter-weight execution mechanism than the `Task` object, there will be some more
restrictions that need to be placed on these locking mechanisms.

## Locking Mechanisms

The most basic solution to solving these issues is to use some sort of lock. The called function will first aquire the
lock before performing its logic, which ensures that race conditions are avoided.

### Spinlock

The most basic mechanism is the simple spinlock, whose locking operations is to repeatedly try to write a 1 to the lock
itself. It can only do so when the lock is 0.

As mentioned above, a spinlock which protects execution during a HW interrupt must disable interrupts inside its body.
As consequence, the Iris kernel defines 2 spinlock types: `Spinlock` and `InterruptibleSpinlock`. Since disabling IRQs
is only a performance pessimization, the default will disable interrupts.

Assuming that HW IRQs can allocate memory, the internal heap and page frame allocators will need to use the interrupt
disabling variants. On the other hand, locks related to tasks and scheduling can likely use the `InterruptibleSpinlock`
type.

### Mutex

The primary issue with a spinlock is that it will never stop trying to aquire the lock, which means it can waste a large
number of CPU cycles. This cost can be mitigated by having kernel level spinlocks prevent the running task from being
pre-empted, but in case where there is a long critical section, it is likely ill-advised to use a spinlock.

A mutex internally has a queue of waiting tasks, and when the mutex is unlocked, the kernel will schedule exactly 1 task
to execute, and this task is expected to acquire the lock.

A mutex cannot be used to synchronize with interrupt handlers, because interrupt handlers do not have an associated task
object for which their execution can be queued. As such, anything accessible from interrupt handlers **MUST** use a
`Spinlock`.

### Note on Asynchronous Execution

Assuming the Iris kernel implements a lighter-weight execution mechanism than the `Task` object, a different type of
mutex will be needed, which operates on the level of a coroutine execution.
