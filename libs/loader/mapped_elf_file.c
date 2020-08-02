#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/param.h>

#include "dynamic_elf_object.h"
#include "mapped_elf_file.h"
#include "tls_record.h"

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

struct mapped_elf_file build_mapped_elf_file(const char *file, int *error) {
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

void destroy_mapped_elf_file(struct mapped_elf_file *self) {
    munmap(self->base, self->size);
}

const Elf64_Ehdr *elf_header(const struct mapped_elf_file *self) {
    return self->base;
}

const Elf64_Shdr *section_table(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    return self->base + header->e_shoff;
}

const Elf64_Shdr *section_at(const struct mapped_elf_file *self, size_t index) {
    return &section_table(self)[index];
}

size_t section_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_shnum;
}

const Elf64_Phdr *program_header_table(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    return self->base + header->e_phoff;
}

const Elf64_Phdr *program_header_at(const struct mapped_elf_file *self, size_t index) {
    return &program_header_table(self)[index];
}

size_t program_header_count(const struct mapped_elf_file *self) {
    return elf_header(self)->e_phnum;
}

const char *section_strings(const struct mapped_elf_file *self) {
    const Elf64_Ehdr *header = elf_header(self);
    if (header->e_shstrndx == 0) {
        return NULL;
    }
    const Elf64_Shdr *string_table = section_at(self, header->e_shstrndx);
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
        const Elf64_Shdr *section = section_at(self, i);
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

const Elf64_Phdr *dynamic_program_header(const struct mapped_elf_file *self) {
    size_t count = program_header_count(self);
    for (size_t i = 0; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(self, i);
        if (phdr->p_type == PT_DYNAMIC) {
            return phdr;
        }
    }
    return NULL;
}

uintptr_t dynamic_table_offset(const struct mapped_elf_file *self) {
    const Elf64_Phdr *phdr = dynamic_program_header(self);
    if (!phdr) {
        return -1;
    }
    return phdr->p_vaddr;
}

size_t dynamic_count(const struct mapped_elf_file *self) {
    const Elf64_Phdr *phdr = dynamic_program_header(self);
    if (!phdr) {
        return 0;
    }
    return phdr->p_filesz / sizeof(Elf64_Dyn);
}

struct dynamic_elf_object *load_mapped_elf_file(struct mapped_elf_file *file) {
    size_t count = program_header_count(file);
    if (count == 0) {
        return NULL;
    }

    const Elf64_Phdr *first = program_header_at(file, 0);
    const Elf64_Phdr *last = program_header_at(file, 0);
    size_t tls_size = 0;
    size_t tls_align = 0;
    for (size_t i = 1; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(file, i);
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

    void *base = mmap(NULL, total_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, 0, 0);
    if ((intptr_t) base < 0 && (intptr_t) base > -300) {
        return NULL;
    }

    void *tls_image = NULL;
    for (size_t i = 0; i < count; i++) {
        const Elf64_Phdr *phdr = program_header_at(file, i);
        if (phdr->p_type == PT_TLS && tls_size) {
            tls_image = base + tls_start;
            memcpy(tls_image, file->base + phdr->p_offset, phdr->p_filesz);
            // mprotect(tls_image, ROUND_UP(tls_size, PAGE_SIZE), PROT_READ);
            continue;
        }

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

    struct tls_record *tls_record = NULL;
    if (tls_image) {
        tls_record = add_tls_record(tls_image, tls_size, tls_align, 0);
    }

    struct dynamic_elf_object *obj = loader_malloc(sizeof(struct dynamic_elf_object));
    *obj = build_dynamic_elf_object(base + dyn_table_offset, dyn_count, base, total_size, (uintptr_t) base, tls_record);

#ifdef LOADER_DEBUG
    loader_log("loaded `%s' at %p", object_name(obj), base);
#endif /* LOADER_DEBUG */

    return obj;
}
