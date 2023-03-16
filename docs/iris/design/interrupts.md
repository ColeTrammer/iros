# Interrupts

## Explanation

An interrupt refers to the currently executing processor stopping its current execution in response to some event. This
usually acts as a mini context switch, where the processor effectively loads new values for the stack, and saves the old
processor state to be resumed later.

## Interrupt Sources

In practice, there are 3 types of interrupt sources.

| Source     | Description                                                             |
| ---------- | ----------------------------------------------------------------------- |
| CPU        | Exception, occurs on page fault or dividing by zero                     |
| Programmed | Caused directly by userspace, on x86 using the `int` instruction        |
| External   | Caused by hardware, needs to be responded to by specific device drivers |

Each kind of source requires slightly different handling. In particular, CPU exceptions do not require interaction with
any external components, but may effect the currently running task. For instance, a page fault would require the kernel
to load memory from disk, potentially blocking the task for a long time.

On the other hand, external interrupts usually require interacting directly with the hardware that caused the interrupt.
Additionally, an interrupt controller device has to be informed the interrupt was handled successfully. These contexts
also have no associated task, and so cannot block. HW interrupt handlers _should_ perform as little work as possible.

Lastly, user specified interrupts are used to invoke system calls. These are performance sensitive, and so can be dealt
with in a special fast path.

## Hardware Interrupts

One difficult aspect of hardware interrupts is that devices can share the same interrupt line. This is especially common
in old PCI devices, since there are only 4 interrupt lines available to all PCI devices (on x86 before MSI). As such,
interrupt handlers must be prepared to encounter spurious interrupts, which do not correspond to an event for the
specific device they are interested in. Even without shared interrupts, spurious interrupts can occur due to hardware
interference/noise.

Another confusing aspect is the actual mechanism by which to determine the interrupt line a device is wired too. For x86
legacy devices, this value is effectively hardcoded, but this is likely not the case in more recent devices.
Additionally, interrupts have to be routed through an interrupt controller, and there can be multiple such devices (IO
APICs) on certain chipsets. And different devices may differentiate between edge-triggered and level-triggered
interrupts.

### NMI

Non-maskable interrupts will always fire, even if the processor has disabled interrupts. The most known cause for this
exception is that the computer's RAM failed in some way. This requires special handling in the kernel, because
technically NMI's can nest, and can also occur at any point in a program's execution.

### Note for Micro-Kernels

In general, we want to respond as quickly as possible to HW interrupts. However, when dealing with out-of-kernel device
drivers, the kernel would typically have to wait for them to finish handling each IRQ before it can exit IRQ context,
and then send the EOI signal to the interrupt controller. As consequence, most micro-kernels allow drivers to load code
whose sole purpose is responding to interrupts in the kernel. This language can be simple, but it is required to allow
drivers to respond to interrupts and also must be provably safe. In addition, a more complex language can be extended to
other tasks, like eBNF is on Linux (note that eBNF is not used for interrupts on Linux, but does have many other
applications).
