#include <elf.h>
#include <sys/param.h>

#include "dynamic_elf_object.h"
#include "mapped_elf_file.h"
#include "tls_record.h"

struct dynamic_elf_object build_dynamic_elf_object(const Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base, size_t size,
                                                   size_t relocation_offset, struct tls_record *tls_record) {
    struct dynamic_elf_object self = { 0 };
    self.tls_record = tls_record;
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

void destroy_dynamic_elf_object(struct dynamic_elf_object *self) {
    if (self->tls_record) {
        remove_tls_record(self->tls_record);
    }
    munmap(self->raw_data, self->raw_data_size);
}

const char *dynamic_strings(const struct dynamic_elf_object *self) {
    return (const char *) (self->string_table + self->relocation_offset);
}

const char *dynamic_string(const struct dynamic_elf_object *self, size_t i) {
    return &dynamic_strings(self)[i];
}

const char *object_name(const struct dynamic_elf_object *self) {
    if (self->so_name_offset) {
        return dynamic_string(self, self->so_name_offset);
    }
    return program_name;
}

size_t rela_count(const struct dynamic_elf_object *self) {
    if (!self->rela_size || !self->rela_entry_size) {
        return 0;
    }
    return self->rela_size / self->rela_entry_size;
}

const Elf64_Rela *rela_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Rela *) (self->rela_addr + self->relocation_offset);
}

const Elf64_Rela *rela_at(const struct dynamic_elf_object *self, size_t i) {
    return &rela_table(self)[i];
}

size_t plt_relocation_count(const struct dynamic_elf_object *self) {
    size_t ent_size = self->plt_type == DT_RELA ? sizeof(Elf64_Rela) : sizeof(Elf64_Rel);

    if (!self->plt_size) {
        return 0;
    }
    return self->plt_size / ent_size;
}

const void *plt_relocation_table(const struct dynamic_elf_object *self) {
    return (void *) (self->plt_addr + self->relocation_offset);
}

const void *plt_relocation_at(const struct dynamic_elf_object *self, size_t i) {
    if (self->plt_type == DT_RELA) {
        const Elf64_Rela *tbl = plt_relocation_table(self);
        return &tbl[i];
    }
    const Elf64_Rel *tbl = plt_relocation_table(self);
    return &tbl[i];
}

const Elf64_Sym *symbol_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Sym *) (self->symbol_table + self->relocation_offset);
}

const Elf64_Sym *symbol_at(const struct dynamic_elf_object *self, size_t i) {
    return &symbol_table(self)[i];
}

const char *symbol_name(const struct dynamic_elf_object *self, size_t i) {
    return dynamic_string(self, symbol_at(self, i)->st_name);
}

const Elf64_Word *hash_table(const struct dynamic_elf_object *self) {
    return (const Elf64_Word *) (self->hash_table + self->relocation_offset);
}

const Elf64_Sym *lookup_symbol(const struct dynamic_elf_object *self, const char *s) {
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

void free_dynamic_elf_object(struct dynamic_elf_object *self) {
    destroy_dynamic_elf_object(self);
    loader_free(self);
}

void call_init_functions(struct dynamic_elf_object *obj, int argc, char **argv, char **envp) {
#ifdef LOADER_DEBUG
    loader_log("doing init functions for `%s'", object_name(obj));
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

void call_fini_functions(struct dynamic_elf_object *obj) {
#ifdef LOADER_DEBUG
    loader_log("doing fini functions for `%s'", object_name(obj));
#endif /* LOADER_DEBUG */

    if (obj->fini_array_size) {
        fini_function_t *fini = (fini_function_t *) (obj->fini_array + obj->relocation_offset);
        for (size_t i = 0; i < obj->fini_array_size / sizeof(fini_function_t); i++) {
            fini[i]();
        }
    }

    if (obj->fini_addr) {
        fini_function_t fini = (fini_function_t)(obj->fini_addr + obj->relocation_offset);
        fini();
    }
}
LOADER_HIDDEN_EXPORT(call_fini_functions, __loader_call_fini_functions);

void add_dynamic_object(struct dynamic_elf_object *obj) {
    insque(obj, dynamic_object_tail);
    dynamic_object_tail = obj;
}

static bool already_loaded(const char *lib_name) {
    struct dynamic_elf_object *obj = dynamic_object_head;
    while (obj) {
        if (strcmp(object_name(obj), lib_name) == 0) {
            return true;
        }
        obj = obj->next;
    }
    return false;
}

void load_dependencies(struct dynamic_elf_object *obj) {
    for (size_t i = 0; i < obj->dependencies_size; i++) {
        const char *lib_name = dynamic_string(obj, obj->dependencies[i]);
        if (already_loaded(lib_name)) {
            continue;
        }

        char path[256];
        strcpy(path, "/usr/lib/");
        strcpy(path + strlen("/usr/lib/"), lib_name);
#ifdef LOADER_DEBUG
        loader_log("Loading dependency of `%s': `%s'", object_name(obj), lib_name);
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
}

struct dynamic_elf_object *get_dynamic_object_head(void) {
    return dynamic_object_head;
}
LOADER_HIDDEN_EXPORT(get_dynamic_object_head, __loader_get_dynamic_object_head);

struct dynamic_elf_object *get_dynamic_object_tail(void) {
    return dynamic_object_tail;
}
LOADER_HIDDEN_EXPORT(get_dynamic_object_tail, __loader_get_dynamic_object_tail);
