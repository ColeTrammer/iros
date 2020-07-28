#include <elf.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>

#include "loader.h"

struct mapped_elf_file {
    void *base;
    size_t size;
};

static int validate_elf_shared_library(void *base, size_t size) {
    if (size < sizeof(Elf64_Ehdr)) {
        return -1;
    }

    const Elf64_Ehdr *header = (const Elf64_Ehdr *) base;
    if (header->e_ident[EI_MAG0] != 0x7F || header->e_ident[EI_MAG1] != 'E' || header->e_ident[EI_MAG2] != 'L' ||
        header->e_ident[EI_MAG3] != 'F') {
        return -1;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        return -1;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return -1;
    }

    if (header->e_ident[EI_VERSION] != EV_CURRENT) {
        return -1;
    }

    if (header->e_ident[EI_OSABI] != ELFOSABI_SYSV || header->e_ident[EI_ABIVERSION] != 0) {
        return -1;
    }

    if (header->e_type != ET_DYN) {
        return -1;
    }

    return 0;
}

static struct mapped_elf_file build_mapped_elf_file(const char *file, int *error) {
    void *base = NULL;
    int fd = open(file, O_RDONLY, 0);
    if (fd < 0) {
        *error = fd;
        goto build_mapped_elf_file_fail;
    }

    struct stat st;
    int ret = fstat(fd, &st);
    if (ret < 0) {
        *error = -1;
        goto build_mapped_elf_file_fail;
    }

    size_t size = st.st_size;
    base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if ((intptr_t) base < 0 && (intptr_t) base > -300) {
        base = NULL;
        *error = -1;
        goto build_mapped_elf_file_fail;
    }

    int validation_error = validate_elf_shared_library(base, size);
    if (validation_error) {
        *error = validation_error;
        goto build_mapped_elf_file_fail;
    }

    ret = close(fd);
    if (ret < 0) {
        fd = -1;
        *error = -1;
        goto build_mapped_elf_file_fail;
    }

    *error = 0;
    return (struct mapped_elf_file) { .base = base, .size = size };

build_mapped_elf_file_fail:
    if (fd >= 0) {
        close(fd);
    }

    if (base) {
        munmap(base, size);
    }

    return (struct mapped_elf_file) { .base = NULL, .size = 0 };
}

static void destroy_mapped_elf_file(struct mapped_elf_file *self) {
    munmap(self->base, self->size);
}

static const Elf64_Ehdr *elf_header(const struct mapped_elf_file *self) {
    return self->base;
}

static const Elf64_Shdr *section_table(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    return self->base + header->e_shoff;
}

static const Elf64_Shdr *section_at(const struct mapped_elf_file *self, size_t index) {
    return &section_table(self)[index];
}

static size_t section_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_shnum;
}

static const Elf64_Phdr *program_header_table(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    return self->base + header->e_phoff;
}

static const Elf64_Phdr *program_header_at(const struct mapped_elf_file *self, size_t index) {
    return &program_header_table(self)[index];
}

static size_t program_header_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_phnum;
}

static const char *section_strings(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    if (header->e_shstrndx == 0) {
        return NULL;
    }
    const Elf64_Shdr *string_table = section_at(self, header->e_shstrndx);
    return self->base + string_table->sh_offset;
}

static const char *section_string(const struct mapped_elf_file *self, size_t index) {
    const char *strings = section_strings(self);
    if (!strings) {
        return NULL;
    }
    return &strings[index];
}

static const char *strings(const struct mapped_elf_file *self) {
    size_t count = section_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Shdr *section = section_at(self, i);
        if (section->sh_type == SHT_STRTAB && strcmp(section_string(self, section->sh_name), ".strtab") == 0) {
            return self->base + section->sh_offset;
        }
    }
    return NULL;
}

static __attribute__((unused)) const char *string(const struct mapped_elf_file *self, size_t index) {
    const char *strs = strings(self);
    if (!strs) {
        return NULL;
    }
    return &strs[index];
}

static const Elf64_Phdr *dynamic_program_header(const struct mapped_elf_file *self) {
    size_t count = program_header_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(self, i);
        if (phdr->p_type == PT_DYNAMIC) {
            return phdr;
        }
    }
    return NULL;
}

static uintptr_t dynamic_table_offset(const struct mapped_elf_file *self) {
    const Elf64_Phdr *phdr = dynamic_program_header(self);
    if (!phdr) {
        return -1;
    }
    return phdr->p_vaddr;
}

static size_t dynamic_count(const struct mapped_elf_file *self) {
    const Elf64_Phdr *phdr = dynamic_program_header(self);
    if (!phdr) {
        return 0;
    }
    return phdr->p_filesz / sizeof(Elf64_Dyn);
}

struct dynamic_elf_object {
    uintptr_t hash_table;
    uintptr_t string_table;
    uintptr_t symbol_table;
    uintptr_t init_addr;
    uintptr_t fini_addr;
    uintptr_t rela_addr;
    uintptr_t plt_addr;
    size_t plt_size;
    size_t plt_type;
    uintptr_t got_addr;
    size_t rela_size;
    size_t rela_entry_size;
    size_t so_name_offset;
    size_t string_table_size;
    size_t symbol_entry_size;
    uint8_t *raw_data;
    size_t *dependencies;
    size_t dependencies_size;
    size_t dependencies_max;
};

static struct dynamic_elf_object build_dynamic_elf_object(const Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base) {
    struct dynamic_elf_object self = { 0 };
    self.raw_data = base;
    for (size_t i = 0; i < dynamic_count; i++) {
        const Elf64_Dyn *entry = &dynamic_table[i];
        switch (entry->d_tag) {
            case DT_NULL:
                return self;
            case DT_NEEDED:
                if (self.dependencies_size >= self.dependencies_max) {
                    self.dependencies_max = MAX(20, self.dependencies_max * 2);
                    self.dependencies = loader_realloc(self.dependencies, self.dependencies_max);
                }
                self.dependencies[self.dependencies_size++] = entry->d_un.d_val;
                break;
            case DT_PLTRELSZ:
                self.plt_size = entry->d_un.d_val;
                break;
            case DT_PLTGOT:
                self.got_addr = entry->d_un.d_ptr;
                break;
            case DT_HASH:
                self.hash_table = entry->d_un.d_ptr;
                break;
            case DT_STRTAB:
                self.string_table = entry->d_un.d_ptr;
                break;
            case DT_SYMTAB:
                self.symbol_table = entry->d_un.d_ptr;
                break;
            case DT_RELA:
                self.rela_addr = entry->d_un.d_ptr;
                break;
            case DT_RELASZ:
                self.rela_size = entry->d_un.d_val;
                break;
            case DT_RELAENT:
                self.rela_entry_size = entry->d_un.d_val;
                break;
            case DT_STRSZ:
                self.string_table_size = entry->d_un.d_val;
                break;
            case DT_SYMENT:
                self.symbol_entry_size = entry->d_un.d_val;
                break;
            case DT_INIT:
                self.init_addr = entry->d_un.d_ptr;
                break;
            case DT_FINI:
                self.fini_addr = entry->d_un.d_ptr;
                break;
            case DT_SONAME:
                self.so_name_offset = entry->d_un.d_val;
                break;
            case DT_RPATH:
                // ignored for now
                break;
            case DT_PLTREL:
                self.plt_type = entry->d_un.d_val;
                break;
            case DT_DEBUG:
                break;
            case DT_TEXTREL:
                break;
            case DT_JMPREL:
                self.plt_addr = entry->d_un.d_ptr;
                break;
            case DT_RELACOUNT:
                break;
            case DT_RELCOUNT:
                break;
            default:
                loader_log("Unkown DT_* value %ld", entry->d_tag);
                break;
        }
    }
    return self;
}

static const char *dynamic_strings(const struct dynamic_elf_object *self) {
    return (const char *) self->string_table;
}

static const char *dynamic_string(const struct dynamic_elf_object *self, size_t i) {
    return &dynamic_strings(self)[i];
}

static size_t rela_count(const struct dynamic_elf_object *self) {
    return self->rela_size / self->rela_entry_size;
}

static const Elf64_Rela *rela_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Rela *) self->rela_addr;
}

static const Elf64_Rela *rela_at(const struct dynamic_elf_object *self, size_t i) {
    return &rela_table(self)[i];
}

static const Elf64_Sym *symbol_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Sym *) self->symbol_table;
}

static const Elf64_Sym *symbol_at(const struct dynamic_elf_object *self, size_t i) {
    return &symbol_table(self)[i];
}

static const char *symbol_name(const struct dynamic_elf_object *self, size_t i) {
    return dynamic_string(self, symbol_at(self, i)->st_name);
}

static const Elf64_Word *hash_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Word *) self->hash_table;
}

static __attribute__((unused)) const Elf64_Sym *lookup_symbol(const struct dynamic_elf_object *self, const char *s) {
    const Elf64_Word *ht = hash_table(self);
    Elf64_Word nbucket = ht[0];
    Elf64_Word nchain = ht[1];
    unsigned long hashed_value = elf64_hash(s);
    unsigned long bucket_index = hashed_value % nbucket;
    Elf64_Word symbol_index = ht[2 + bucket_index];
    while (symbol_index != STN_UNDEF) {
        if (strcmp(symbol_name(self, symbol_index), s) == 0) {
            return symbol_at(self, symbol_index);
        }

        size_t chain_index = 2 + nbucket + symbol_index;
        if (symbol_index >= nchain) {
            return NULL;
        }
        symbol_index = ht[chain_index];
    }
    return NULL;
}

static __attribute__((unused)) void process_relocations(const struct dynamic_elf_object *self) {
    for (size_t i = 0; i < rela_count(self); i++) {
        const Elf64_Rela *rela = rela_at(self, i);
        size_t type = ELF64_R_TYPE(rela->r_info);
        size_t symbol_index = ELF64_R_SYM(rela->r_info);
        switch (type) {
                // A   - The addend used to compute the value of the relocatable field.
                // B   - The base address at which a shared object is loaded into memory during execution. Generally, a shared object
                // file
                //       is built with a base virtual address of 0. However, the execution address of the shared object is different.
                //       See Program Header.
                // G   - The offset into the global offset table at which the address of the relocation entry's symbol resides during
                //       execution.
                // GOT - The address of the global offset table.
                // L   - The section offset or address of the procedure linkage table entry for a symbol.
                // P   - The section offset or address of the storage unit being relocated, computed using r_offset.
                // S   - The value of the symbol whose index resides in the relocation entry.
                // Z   - The size of the symbol whose index resides in the relocation entry.
            case R_X86_64_NONE:
                break;
            case R_X86_64_64:
                loader_log("R_X86_64_64 for symbol %s\n", symbol_name(self, symbol_index));
                break;
            case R_X86_64_GLOB_DAT:
                loader_log("R_X86_64_GLOB_DATA for symbol %s\n", symbol_name(self, symbol_index));
                break;
            case R_X86_64_RELATIVE: {
                // B + A
                uintptr_t B = (uintptr_t) self->raw_data;
                uintptr_t A = rela->r_addend;
                uint64_t *addr = (uint64_t *) (self->raw_data + rela->r_offset);
                *addr = B + A;
                break;
            }
            default:
                loader_log("Unkown relocation type %ld\n", type);
                break;
        }
    }
}

void _entry(struct initial_process_info *info, int argc, char **argv, char **envp) {
    struct dynamic_elf_object program = build_dynamic_elf_object(
        (const Elf64_Dyn *) info->program_dynamic_start, info->program_dynamic_size / sizeof(Elf64_Dyn), (uint8_t *) info->program_offset);
    for (size_t i = 0; i < program.dependencies_size; i++) {
        char path[256];
        strcpy(path, "/usr/lib/");
        strcpy(path + strlen("/usr/lib/"), dynamic_string(&program, program.dependencies[i]));
        loader_log("Loading dependency of the `%s': `%s'", *argv, path);

        int error;
        struct mapped_elf_file lib = build_mapped_elf_file(path, &error);
        if (error) {
            loader_log("Failed to load dependency\n");
            _exit(98);
        }

        size_t dyn_count = dynamic_count(&lib);
        uintptr_t dyn_table_offset = dynamic_table_offset(&lib);
        (void) dyn_count;
        (void) dyn_table_offset;

        destroy_mapped_elf_file(&lib);
    }

    loader_log("running program");
    asm volatile("and $(~16), %%rsp\n"
                 "mov %0, %%rdi\n"
                 "mov %1, %%esi\n"
                 "mov %2, %%rdx\n"
                 "mov %3, %%rcx\n"
                 "jmp *%4\n"
                 :
                 : "r"(info), "r"(argc), "r"(argv), "r"(envp), "r"(info->program_entry)
                 : "rdi", "rsi", "rdx", "rcx", "memory");
    _exit(99);
}
