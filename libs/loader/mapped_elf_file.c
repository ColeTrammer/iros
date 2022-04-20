#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/param.h>

#include "dynamic_elf_object.h"
#include "mapped_elf_file.h"
#include "tls_record.h"

static int validate_elf_shared_library(void *base, size_t size) {
    if (size < sizeof(ElfW(Ehdr))) {
        return -1;
    }

    const ElfW(Ehdr) *header = (const ElfW(Ehdr) *) base;
    if (header->e_ident[EI_MAG0] != 0x7F || header->e_ident[EI_MAG1] != 'E' || header->e_ident[EI_MAG2] != 'L' ||
        header->e_ident[EI_MAG3] != 'F') {
        return -1;
    }

#ifdef __x86_64__
    uint8_t expected_class = ELFCLASS64;
#else
    uint8_t expected_class = ELFCLASS32;
#endif
    if (header->e_ident[EI_CLASS] != expected_class) {
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

struct mapped_elf_file build_mapped_elf_file(const char *file) {
    void *base = NULL;
    int fd = open(file, O_RDONLY, 0);
    if (fd < 0) {
        loader_err("Could not open `%s'", file);
        goto build_mapped_elf_file_fail;
    }

    struct stat st;
    int ret = fstat(fd, &st);
    if (ret < 0) {
        loader_err("Could not stat `%s'", file);
        goto build_mapped_elf_file_fail;
    }

    size_t size = st.st_size;
    base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if ((intptr_t) base < 0 && (intptr_t) base > -300) {
        base = NULL;
        loader_err("Could not mmap `%s'", file);
        goto build_mapped_elf_file_fail;
    }

    if (validate_elf_shared_library(base, size)) {
        loader_err("`%s' is not a valid elf file", file);
        goto build_mapped_elf_file_fail;
    }

    return (struct mapped_elf_file) { .base = base, .size = size, .fd = fd };

build_mapped_elf_file_fail:
    if (fd >= 0) {
        close(fd);
    }

    if (base) {
        munmap(base, size);
    }

    return (struct mapped_elf_file) { .base = NULL, .size = 0, .fd = -1 };
}
LOADER_HIDDEN_EXPORT(build_mapped_elf_file, __loader_build_mapped_elf_file);

void destroy_mapped_elf_file(struct mapped_elf_file *self) {
    if (self->base) {
        munmap(self->base, self->size);
    }
    if (self->fd != -1) {
        close(self->fd);
    }
}
LOADER_HIDDEN_EXPORT(destroy_mapped_elf_file, __loader_destroy_mapped_elf_file);

const ElfW(Ehdr) * elf_header(const struct mapped_elf_file *self) {
    return self->base;
}

const ElfW(Shdr) * section_table(const struct mapped_elf_file *self) {
    const ElfW(Ehdr) *header = elf_header(self);
    return self->base + header->e_shoff;
}

const ElfW(Shdr) * section_at(const struct mapped_elf_file *self, size_t index) {
    return &section_table(self)[index];
}

size_t section_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_shnum;
}

const ElfW(Phdr) * program_header_table(const struct mapped_elf_file *self) {
    const ElfW(Ehdr) *header = elf_header(self);
    return self->base + header->e_phoff;
}

const ElfW(Phdr) * program_header_at(const struct mapped_elf_file *self, size_t index) {
    return &program_header_table(self)[index];
}

size_t program_header_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_phnum;
}

const char *section_strings(const struct mapped_elf_file *self) {
    const ElfW(Ehdr) *header = elf_header(self);
    if (header->e_shstrndx == 0) {
        return NULL;
    }
    const ElfW(Shdr) *string_table = section_at(self, header->e_shstrndx);
    return self->base + string_table->sh_offset;
}

const char *section_string(const struct mapped_elf_file *self, size_t index) {
    const char *strings = section_strings(self);
    if (!strings) {
        return NULL;
    }
    return &strings[index];
}

const char *strings(const struct mapped_elf_file *self) {
    size_t count = section_count(self);
    for (size_t i = 0; i < count; i++) {
        const ElfW(Shdr) *section = section_at(self, i);
        if (section->sh_type == SHT_STRTAB && strcmp(section_string(self, section->sh_name), ".strtab") == 0) {
            return self->base + section->sh_offset;
        }
    }
    return NULL;
}

__attribute__((unused)) const char *string(const struct mapped_elf_file *self, size_t index) {
    const char *strs = strings(self);
    if (!strs) {
        return NULL;
    }
    return &strs[index];
}

const ElfW(Phdr) * dynamic_program_header(const struct mapped_elf_file *self) {
    size_t count = program_header_count(self);
    for (size_t i = 0; i < count; i++) {
        const ElfW(Phdr) *phdr = program_header_at(self, i);
        if (phdr->p_type == PT_DYNAMIC) {
            return phdr;
        }
    }
    return NULL;
}

uintptr_t dynamic_table_offset(const struct mapped_elf_file *self) {
    const ElfW(Phdr) *phdr = dynamic_program_header(self);
    if (!phdr) {
        return -1;
    }
    return phdr->p_vaddr;
}

size_t dynamic_count(const struct mapped_elf_file *self) {
    const ElfW(Phdr) *phdr = dynamic_program_header(self);
    if (!phdr) {
        return 0;
    }
    return phdr->p_filesz / sizeof(ElfW(Dyn));
}

struct dynamic_elf_object *load_mapped_elf_file(struct mapped_elf_file *file, const char *full_path, bool global, bool use_initial_tls) {
    size_t count = program_header_count(file);
    if (count == 0) {
        loader_err("`%s' has no program headers", full_path);
        return NULL;
    }

    const ElfW(Phdr) *first = program_header_at(file, 0);
    const ElfW(Phdr) *last = program_header_at(file, 0);
    size_t tls_size = 0;
    size_t tls_align = 0;
    for (size_t i = 1; i < count; i++) {
        const ElfW(Phdr) *phdr = program_header_at(file, i);
        if (phdr->p_type == PT_TLS) {
            tls_size = phdr->p_memsz;
            tls_align = phdr->p_align;
            continue;
        }

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
    total_size = ALIGN_UP(total_size, PAGE_SIZE);
    size_t tls_start = total_size;
    total_size += tls_size;
    total_size = ALIGN_UP(total_size, PAGE_SIZE);

    void *base = mmap(NULL, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, 0, 0);
    if ((intptr_t) base < 0 && (intptr_t) base > -300) {
        loader_err("Could not resserve space for `%s' with mmap", full_path);
        return NULL;
    }

    void *tls_image = NULL;
    for (size_t i = 0; i < count; i++) {
        const ElfW(Phdr) *phdr = program_header_at(file, i);
        if (phdr->p_type == PT_TLS && tls_size) {
            tls_image = base + tls_start;
            mprotect(tls_image, ALIGN_UP(tls_size, PAGE_SIZE), PROT_WRITE);
            memcpy(tls_image, file->base + phdr->p_offset, phdr->p_filesz);
            mprotect(tls_image, ALIGN_UP(tls_size, PAGE_SIZE), PROT_READ);
            continue;
        }

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        int prot =
            (phdr->p_flags & PF_R ? PROT_READ : 0) | (phdr->p_flags & PF_W ? PROT_WRITE : 0) | (phdr->p_flags & PF_X ? PROT_EXEC : 0);

        void *phdr_start = base + phdr->p_vaddr;
        void *phdr_end = (void *) ALIGN_UP((uintptr_t) phdr_start + phdr->p_memsz, PAGE_SIZE);
        void *phdr_page_start = (void *) (((uintptr_t) phdr_start) & ~(PAGE_SIZE - 1));

        if ((phdr->p_filesz == phdr->p_memsz) && !(prot & PROT_WRITE) && (phdr->p_align % PAGE_SIZE == 0)) {
            mmap(phdr_start, phdr_end - phdr_start, prot | PROT_WRITE, MAP_SHARED | MAP_FIXED, file->fd, phdr->p_offset);
        } else {
            mprotect(phdr_page_start, phdr_end - phdr_start, PROT_WRITE);
            memcpy(phdr_start, file->base + phdr->p_offset, phdr->p_filesz);
            mprotect(phdr_page_start, phdr_end - phdr_start, prot);
        }
    }

    size_t dyn_count = dynamic_count(file);
    uintptr_t dyn_table_offset = dynamic_table_offset(file);

    size_t tls_module_id = 0;
    if (tls_image) {
        tls_module_id = add_tls_record(tls_image, tls_size, tls_align, use_initial_tls ? TLS_INITIAL_IMAGE : 0);
    }

    struct dynamic_elf_object *obj = loader_malloc(sizeof(struct dynamic_elf_object));
    void *phdr_start = loader_malloc(count * sizeof(ElfW(Phdr)));
    memcpy(phdr_start, program_header_table(file), count * sizeof(ElfW(Phdr)));
    *obj = build_dynamic_elf_object(base + dyn_table_offset, dyn_count, base, total_size, (uintptr_t) base, phdr_start, count,
                                    tls_module_id, full_path, global);

#ifdef LOADER_DEBUG
    loader_log("loaded `%s' at %p", object_name(obj), base);
#endif /* LOADER_DEBUG */

    add_dynamic_object(obj);
    return obj;
}
LOADER_HIDDEN_EXPORT(load_mapped_elf_file, __loader_load_mapped_elf_file);
