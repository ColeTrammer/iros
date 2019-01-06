#ifndef _KERNEL_MEM_KERNE_VM_H
#define _KERNEL_MEM_KERNE_VM_H 1

#include <stdint.h>

extern void KERNEL_VMA();

#define KERNEL_VM_START ((uintptr_t) &KERNEL_VMA)

#endif /* _KERNEL_MEM_KERNE_VM_H */