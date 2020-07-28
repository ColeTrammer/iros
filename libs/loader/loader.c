#include <elf.h>
#include <sys/param.h>

#include "loader.h"

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
        loader_log("NEEDS `%s'", dynamic_string(&program, program.dependencies[i]));
    }

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
