# Linux Startup

This document describes the required steps and ABI used to start a Linux executable, from the perspective of the C/C++
runtime. This assumes a statically linked executable for now.

## Entry

The entry point for the program is the is the symbol `_start`. argc, argv, and envp are arranged on the stack as
follows.

```
+----------------+
| ...            |
| elf aux vec    |
+----------------+
| null           |
| ...            |
| envp[0]        | <- rsp + 8 * (argc + 2)
+----------------+
| null           | <- rsp + 8 * (argc + 1)
| argv[argc - 1] |
| ...            |
| argv[0]        | <- rsp + 8
+----------------+
| argc           | <- rsp
+----------------+
```

In particular, the stack is setup such that the top of the stack is argc, and the next 8 byte word is argv. This is null
terminated, and the envp follows. After the envp, comes the ELF auxillary vector, which contains extra information from
the Linux kernel.

## Startup Steps

1. Zero the stack base pointer
2. Setup argc and argv by placing into registers
3. Ensure the stack is 16 byte aligned
4. Call into c++ initialization code
5. Setup thread-local storage
6. Call global constructors
7. Call main

## Setup Thread-Local Storage

Setting up thread-local storage involves allocating some specific memory area with correct alignment, setting up a
thread control block, and calling `arch_prctl(ARCH_SET_FS)` pointing to the middle of this memory area. The challenging
part is to figure out where and and how large the `TLS` segment is. On Linux, this requires parsing the program's own
ELF program headers. For a statically linked executable, this can be done by following the magic `__ehdr_start` symbol,
which points to the program's own ELF header. From there, the runtime must identify if a `TLS` type segment is present,
and if so, reserve space for the segment.

If this is managed, thread-local storage can be allocated as described on
[OSDev](https://wiki.osdev.org/Thread_Local_Storage). However, the exact algorithm used is wrong. In particular, the TLS
data block is always directly before the thread control block, regardless of alignment.

### Is There Anything Simpler?

The Linux kernel provides an auxillary vector which contains extra program information. But this vector does not contain
any information about the thread-local storage segment. It does provide another way to access the program's own program
headers, but I do not know if this entry is present for statically linked executables. It could only be present if a
program header entry of type `PHDR` is present in the ELF binary.

Presumably, this configuration is required because the Linux kernel predates the invention of thread local storage. As
such, there was a goal to minimize the need for kernel changes when adopting this new feature. This partially makes
sense, because it is up to userspace to ultimately manage the thread-local storage. And things get even more complicated
when considering dynamic linking, because multiple libraries can allocate their thread-local regions. Additionally,
access to the program headers is somewhat mandated by the `dl_iterator_phdr` function, which is used to unwind the stack
by libunwind. If userspace has to parse its own program headers anyway, why should Linux change its ABI?

It is tempting to have the Iris kernel provide userspace with the TLS base, TLS alignment, and TLS size based on the
loaded executable. However, this does open up some questions, like: What happens if there are multiple TLS segments?
What happens if the dynamic linker requests TLS? Given that the dius runtime will already support the ABI present on
Linux, it will be easier to just ignore TLS in the kernel. This aligns which its minimalist philosophy.
