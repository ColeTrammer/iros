#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <unistd.h>

#include "loader.h"

#define LOADER_DEBUG
// #define LOADER_SYMBOL_DEBUG

struct dynamic_elf_object;

struct symbol_lookup_result {
    const Elf64_Sym *symbol;
    const struct dynamic_elf_object *object;
};

#define SYMBOL_LOOKUP_NOT_CURRENT 1

static struct symbol_lookup_result do_symbol_lookup(const char *s, const struct dynamic_elf_object *current_object, int flags);

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
    struct dynamic_elf_object *next;
    struct dynamic_elf_object *prev;
    uintptr_t hash_table;
    uintptr_t string_table;
    uintptr_t symbol_table;
    uintptr_t preinit_array;
    size_t preinit_array_size;
    uintptr_t init_addr;
    uintptr_t init_array;
    uintptr_t init_array_size;
    uintptr_t fini_array;
    uintptr_t fini_array_size;
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
    size_t raw_data_size;
    uintptr_t relocation_offset;
    size_t *dependencies;
    size_t dependencies_size;
    size_t dependencies_max;
};

static struct dynamic_elf_object *dynamic_object_head;
static struct dynamic_elf_object *dynamic_object_tail;

static struct dynamic_elf_object build_dynamic_elf_object(const Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base, size_t size,
                                                          size_t relocation_offset) {
    struct dynamic_elf_object self = { 0 };
    self.raw_data = base;
    self.raw_data_size = size;
    self.relocation_offset = relocation_offset;
    for (size_t i = 0; i < dynamic_count; i++) {
        const Elf64_Dyn *entry = &dynamic_table[i];
        switch (entry->d_tag) {
            case DT_NULL:
                return self;
            case DT_NEEDED:
                if (self.dependencies_size >= self.dependencies_max) {
                    self.dependencies_max = MAX(20, self.dependencies_max * 2);
                    self.dependencies = loader_realloc(self.dependencies, self.dependencies_max * sizeof(size_t));
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
            case DT_INIT_ARRAY:
                self.init_array = entry->d_un.d_ptr;
                break;
            case DT_FINI_ARRAY:
                self.fini_array = entry->d_un.d_ptr;
                break;
            case DT_INIT_ARRAYSZ:
                self.init_array_size = entry->d_un.d_val;
                break;
            case DT_FINI_ARRAYSZ:
                self.fini_array_size = entry->d_un.d_val;
                break;
            case DT_PREINIT_ARRAY:
                self.preinit_array = entry->d_un.d_ptr;
                break;
            case DT_PREINIT_ARRAY_SZ:
                self.preinit_array_size = entry->d_un.d_val;
                break;
            case DT_VERSYM:
                break;
            case DT_RELACOUNT:
                break;
            case DT_RELCOUNT:
                break;
            case DT_VERDEF:
                break;
            case DT_VERDEFNUM:
                break;
            case DT_VERNEED:
                break;
            case DT_VERNEEDNUM:
                break;
            default:
                loader_log("Unkown DT_* value %ld", entry->d_tag);
                break;
        }
    }
    return self;
}

static void destroy_dynamic_elf_object(struct dynamic_elf_object *self) {
    munmap(self->raw_data, self->raw_data_size);
}

static const char *dynamic_strings(const struct dynamic_elf_object *self) {
    return (const char *) (self->string_table + self->relocation_offset);
}

static const char *dynamic_string(const struct dynamic_elf_object *self, size_t i) {
    return &dynamic_strings(self)[i];
}

static size_t rela_count(const struct dynamic_elf_object *self) {
    if (!self->rela_size || !self->rela_entry_size) {
        return 0;
    }
    return self->rela_size / self->rela_entry_size;
}

static const Elf64_Rela *rela_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Rela *) (self->rela_addr + self->relocation_offset);
}

static const Elf64_Rela *rela_at(const struct dynamic_elf_object *self, size_t i) {
    return &rela_table(self)[i];
}

static __attribute__((used)) size_t plt_relocation_count(const struct dynamic_elf_object *self) {
    size_t ent_size = self->plt_type == DT_RELA ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel);

    if (!self->plt_size) {
        return 0;
    }
    return self->plt_size / ent_size;
}

static const void *plt_relocation_table(const struct dynamic_elf_object *self) {
    return (void *) (self->plt_addr + self->relocation_offset);
}

static __attribute__((used)) const void *plt_relocation_at(const struct dynamic_elf_object *self, size_t i) {
    if (self->plt_type == DT_RELA) {
        const Elf64_Rela *tbl = plt_relocation_table(self);
        return &tbl[i];
    }
    const Elf64_Rel *tbl = plt_relocation_table(self);
    return &tbl[i];
}

static const Elf64_Sym *symbol_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Sym *) (self->symbol_table + self->relocation_offset);
}

static const Elf64_Sym *symbol_at(const struct dynamic_elf_object *self, size_t i) {
    return &symbol_table(self)[i];
}

static const char *symbol_name(const struct dynamic_elf_object *self, size_t i) {
    return dynamic_string(self, symbol_at(self, i)->st_name);
}

static const Elf64_Word *hash_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Word *) (self->hash_table + self->relocation_offset);
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

static void do_rela(const struct dynamic_elf_object *self, const Elf64_Rela *rela) {
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
        case R_X86_64_64: {
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }

            uint64_t *addr = (uint64_t *) (rela->r_offset + self->relocation_offset);
            *addr = symbol_value + rela->r_addend;
            break;
        }
        case R_X86_64_COPY: {
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, SYMBOL_LOOKUP_NOT_CURRENT);
            if (!result.symbol) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }
            void *dest = (void *) (self->relocation_offset + rela->r_offset);
            const void *src = (const void *) (result.symbol->st_value + result.object->relocation_offset);
            memcpy(dest, src, result.symbol->st_size);
            break;
        }
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT: {
            const Elf64_Sym *symbol_to_lookup = symbol_at(self, symbol_index);
            const char *to_lookup = symbol_name(self, symbol_index);
            struct symbol_lookup_result result = do_symbol_lookup(to_lookup, self, 0);
            uintptr_t symbol_value = 0;
            if (result.symbol) {
                symbol_value = result.symbol->st_value + result.object->relocation_offset;
            } else if (!((symbol_to_lookup->st_info >> 4) & STB_WEAK)) {
                loader_log("Cannot resolve `%s'", to_lookup);
                _exit(97);
            }

            uint64_t *addr = (uint64_t *) (rela->r_offset + self->relocation_offset);
            *addr = symbol_value;
            break;
        }
        case R_X86_64_RELATIVE: {
            // B + A
            uintptr_t B = (uintptr_t) self->relocation_offset;
            uintptr_t A = rela->r_addend;
            uint64_t *addr = (uint64_t *) (self->relocation_offset + rela->r_offset);
            *addr = B + A;
            break;
        }
        default:
            loader_log("Unkown relocation type %ld", type);
            break;
    }
}

extern void got_resolver(void) LOADER_PRIVATE;

LOADER_PRIVATE uintptr_t do_got_resolve(const struct dynamic_elf_object *obj, size_t plt_offset) {
    const Elf64_Rela *relocation = plt_relocation_at(obj, plt_offset);
    const char *to_lookup = symbol_name(obj, ELF64_R_SYM(relocation->r_info));
    struct symbol_lookup_result result = do_symbol_lookup(to_lookup, obj, 0);
    if (!result.symbol) {
        loader_log("Cannot resolve `%s'", to_lookup);
        _exit(96);
    }
    uint64_t *addr = (uint64_t *) (obj->relocation_offset + relocation->r_offset);
    uintptr_t resolved_value = result.symbol->st_value + result.object->relocation_offset;
    *addr = resolved_value;
    return resolved_value;
}

static void process_relocations(const struct dynamic_elf_object *self) {
    size_t count = rela_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Rela *rela = rela_at(self, i);
        do_rela(self, rela);
    }

    if (self->got_addr) {
        uintptr_t *got = (uintptr_t *) (self->got_addr + self->relocation_offset);
        got[1] = (uintptr_t) self;
        got[2] = (uintptr_t) &got_resolver;
    }

#if 1
    size_t plt_count = plt_relocation_count(self);
    for (size_t i = 0; i < plt_count; i++) {
        const Elf64_Rela *rela = plt_relocation_at(self, i);
        do_rela(self, rela);
    }
#endif
}

static struct dynamic_elf_object *load_mapped_elf_file(struct mapped_elf_file *file) {
    size_t count = program_header_count(file);
    if (count == 0) {
        return NULL;
    }

    const Elf64_Phdr *first = program_header_at(file, 0);
    const Elf64_Phdr *last = program_header_at(file, 0);
    for (size_t i = 1; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(file, i);
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        if (phdr->p_vaddr < first->p_vaddr) {
            first = phdr;
        }
        if (phdr->p_vaddr + phdr->p_memsz > last->p_vaddr + last->p_memsz) {
            last = phdr;
        }
    }

    size_t total_size = last->p_vaddr + last->p_memsz;
    total_size = ((total_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    void *base = mmap(NULL, total_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);
    if ((intptr_t) base < 0 && (intptr_t) base > -300) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(file, i);
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        // size_t size = ((phdr->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        void *phdr_start = base + phdr->p_vaddr;
        memcpy(phdr_start, file->base + phdr->p_offset, phdr->p_filesz);
        // void *phdr_page_start = (void *) (((uintptr_t) phdr_start) & ~(PAGE_SIZE - 1));
        // int prot =
        // (phdr->p_flags & PF_R ? PROT_READ : 0) | (phdr->p_flags & PF_W ? PROT_WRITE : 0) | (phdr->p_flags & PF_X ? PROT_EXEC : 0);
        // FIXME: make relocations work without this hack
        // prot |= PROT_WRITE;
        // mprotect(phdr_page_start, size, prot);
    }

    size_t dyn_count = dynamic_count(file);
    uintptr_t dyn_table_offset = dynamic_table_offset(file);

    struct dynamic_elf_object *obj = loader_malloc(sizeof(struct dynamic_elf_object));
    *obj = build_dynamic_elf_object(base + dyn_table_offset, dyn_count, base, total_size, (uintptr_t) base);
    return obj;
}

static __attribute__((unused)) void free_dynamic_elf_object(struct dynamic_elf_object *self) {
    destroy_dynamic_elf_object(self);
    loader_free(self);
}

static struct symbol_lookup_result do_symbol_lookup(const char *s, const struct dynamic_elf_object *current_object, int flags) {
    struct dynamic_elf_object *obj = dynamic_object_head;
#ifdef LOADER_SYMBOL_DEBUG
    loader_log("looking up `%s' for `%s'", s, dynamic_string(current_object, current_object->so_name_offset));
#endif /* LOADER_SYMBOL_DEBUG */
    const Elf64_Sym *weak_symbol = NULL;
    struct dynamic_elf_object *weak_symbol_object = NULL;
    while (obj) {
        if (!(flags & SYMBOL_LOOKUP_NOT_CURRENT) || (obj != current_object)) {
            const Elf64_Sym *sym = lookup_symbol(obj, s);
            if (sym && sym->st_shndx != STN_UNDEF) {
                uint8_t visibility = sym->st_info >> 4;
                if (visibility == STB_GLOBAL) {
                    return (struct symbol_lookup_result) { .symbol = sym, .object = obj };
                } else if (visibility == STB_WEAK && !weak_symbol) {
                    weak_symbol = sym;
                    weak_symbol_object = obj;
                }
            }
        }
        obj = obj->next;
    }
    return (struct symbol_lookup_result) { .symbol = weak_symbol, .object = weak_symbol_object };
}

typedef void (*init_function_t)(int argc, char **argv, char **envp);

static void call_init_functions(struct dynamic_elf_object *obj, int argc, char **argv, char **envp) {
#ifdef LOADER_DEBUG
    loader_log("doing init functions for `%s'", dynamic_string(obj, obj->so_name_offset));
#endif /* LOADER_DEBUG */

    if (obj->preinit_array_size) {
        init_function_t *preinit = (init_function_t *) (obj->preinit_array + obj->relocation_offset);
        for (size_t i = 0; i < obj->preinit_array_size / sizeof(init_function_t); i++) {
            preinit[i](argc, argv, envp);
        }
    }

    if (obj->init_addr) {
        init_function_t init = (init_function_t)(obj->init_addr + obj->relocation_offset);
        init(argc, argv, envp);
    }

    if (obj->init_array_size) {
        init_function_t *init = (init_function_t *) (obj->init_array + obj->relocation_offset);
        for (size_t i = 0; i < obj->init_array_size / sizeof(init_function_t); i++) {
            init[i](argc, argv, envp);
        }
    }
}

static void add_dynamic_object(struct dynamic_elf_object *obj) {
    insque(obj, dynamic_object_tail);
    dynamic_object_tail = obj;
}

void LOADER_PRIVATE _entry(struct initial_process_info *info, int argc, char **argv, char **envp) {
    struct dynamic_elf_object program =
        build_dynamic_elf_object((const Elf64_Dyn *) info->program_dynamic_start, info->program_dynamic_size / sizeof(Elf64_Dyn),
                                 (uint8_t *) info->program_offset, info->program_size, 0);
    dynamic_object_head = dynamic_object_tail = &program;

    for (size_t i = 0; i < program.dependencies_size; i++) {
        char path[256];
        strcpy(path, "/usr/lib/");
        strcpy(path + strlen("/usr/lib/"), dynamic_string(&program, program.dependencies[i]));
#ifdef LOADER_DEBUG
        loader_log("Loading dependency of `%s': `%s'", *argv, path);
#endif /* LOADER_DEBUG */

        int error;
        struct mapped_elf_file lib = build_mapped_elf_file(path, &error);
        if (error) {
            loader_log("Failed to load dependency");
            _exit(98);
        }

        struct dynamic_elf_object *loaded_lib = load_mapped_elf_file(&lib);
        if (!loaded_lib) {
            loader_log("Failed to map dependecy");
            _exit(96);
        }

        add_dynamic_object(loaded_lib);
        destroy_mapped_elf_file(&lib);
    }

    struct dynamic_elf_object *obj = dynamic_object_head;
    while (obj) {
        process_relocations(obj);
        obj = obj->next;
    }

    obj = dynamic_object_tail;
    while (obj) {
        call_init_functions(obj, argc, argv, envp);
        obj = obj->prev;
    }

    loader_log("starting program");
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
