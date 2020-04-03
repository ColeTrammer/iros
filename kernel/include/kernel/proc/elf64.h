#ifndef _KERNEL_PROC_ELF64_H
#define _KERNEL_PROC_ELF64_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

struct file;

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef struct {
    unsigned char e_ident[16]; /* ELF identification */
    Elf64_Half e_type;         /* Object file type */
    Elf64_Half e_machine;      /* Machine type */
    Elf64_Word e_version;      /* Object file version */
    Elf64_Addr e_entry;        /* Entry point address */
    Elf64_Off e_phoff;         /* Program header offset */
    Elf64_Off e_shoff;         /* Section header offset */
    Elf64_Word e_flags;        /* Processor-specific flags */
    Elf64_Half e_ehsize;       /* ELF header size */
    Elf64_Half e_phentsize;    /* Size of program header entry */
    Elf64_Half e_phnum;        /* Number of program header entries */
    Elf64_Half e_shentsize;    /* Size of section header entry */
    Elf64_Half e_shnum;        /* Number of section header entries */
    Elf64_Half e_shstrndx;     /* Section name string table index */
} __attribute__((packed)) Elf64_Ehdr;

typedef struct {
    Elf64_Word p_type;    /* Type of segment */
    Elf64_Word p_flags;   /* Segment attributes */
    Elf64_Off p_offset;   /* Offset in file */
    Elf64_Addr p_vaddr;   /* Virtual address in memory */
    Elf64_Addr p_paddr;   /* Reserved */
    Elf64_Xword p_filesz; /* Size of segment in file */
    Elf64_Xword p_memsz;  /* Size of segment in memory */
    Elf64_Xword p_align;  /* Alignment of segment */
} __attribute__((packed)) Elf64_Phdr;

#define ELF64_PROGRAM_BITS (1)
#define ELF64_NO_BITS      (8)

#define ELF64_WRITE (1 << 0)
#define ELF64_ALLOC (1 << 1)
#define ELF64_EXEC  (1 << 2)

#define ELF64_MAGIC ("\x7F\x45\x4c\x46\x02\x01\x01")

typedef struct {
    Elf64_Word sh_name;       /* Section name */
    Elf64_Word sh_type;       /* Section type */
    Elf64_Xword sh_flags;     /* Section attributes */
    Elf64_Addr sh_addr;       /* Virtual address in memory */
    Elf64_Off sh_offset;      /* Offset in file */
    Elf64_Xword sh_size;      /* Size of section */
    Elf64_Word sh_link;       /* Link to other section */
    Elf64_Word sh_info;       /* Miscellaneous information */
    Elf64_Xword sh_addralign; /* Address alignment boundary */
    Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
} __attribute__((packed)) Elf64_Shdr;

typedef struct {
    Elf64_Word st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} __attribute__((packed)) Elf64_Sym;

bool elf64_is_valid(void *buffer);
uintptr_t elf64_get_start(void *buffer);
uintptr_t elf64_get_entry(void *buffer);
uint64_t elf64_get_size(void *buffer);

void elf64_load_program(void *buffer, size_t length, struct file *executable, struct task *task);
void elf64_map_heap(void *buffer, struct task *task);
struct vm_region *elf64_create_vm_region(void *buffer, uint64_t type);
void elf64_stack_trace(struct task *task);

void kernel_stack_trace(uintptr_t instruction_pointer, uintptr_t frame_base);
void init_kernel_symbols(void);

#endif /* _KERNEL_PROC_ELF64_H */