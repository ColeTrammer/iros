#include <dirent.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "elf_file.h"

static HashMap<FileId, ElfFile*> s_elf_file_map;

SharedPtr<ElfFile> ElfFile::find_or_create(FileId id) {
    auto* ret = s_elf_file_map.get(id);
    if (ret) {
        return (*ret)->shared_from_this();
    }

    const char* search_paths[] = { ".", "/usr/lib" };
    for (auto* path : search_paths) {
        DIR* d = opendir(path);
        if (!d) {
            continue;
        }

        dirent* ent;
        while ((ent = readdir(d))) {
            struct stat st;
            if (fstatat(dirfd(d), ent->d_name, &st, 0)) {
                continue;
            }

            if (st.st_ino != id.inode_id || st.st_dev != id.fs_id) {
                continue;
            }

            auto result = ElfFile::create(id, String::format("%s/%s", path, ent->d_name));
            if (result) {
                return result;
            }
        }
        closedir(d);
    }

    return nullptr;
}

static bool validate_elf(const Ext::MappedFile& file) {
    if (file.size() < sizeof(Elf64_Ehdr)) {
        return false;
    }

    const Elf64_Ehdr* header = (const Elf64_Ehdr*) file.data();
    if (header->e_ident[EI_MAG0] != 0x7F || header->e_ident[EI_MAG1] != 'E' || header->e_ident[EI_MAG2] != 'L' ||
        header->e_ident[EI_MAG3] != 'F') {
        return false;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        return false;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return false;
    }

    if (header->e_ident[EI_VERSION] != EV_CURRENT) {
        return false;
    }

    if (header->e_ident[EI_OSABI] != ELFOSABI_SYSV || header->e_ident[EI_ABIVERSION] != 0) {
        return false;
    }

    return true;
}

SharedPtr<ElfFile> ElfFile::create(FileId file_id, const String& path) {
    auto mapped_file = Ext::MappedFile::create(path, PROT_READ, MAP_SHARED);
    if (!mapped_file) {
        return nullptr;
    }

    if (!validate_elf(*mapped_file)) {
        return nullptr;
    }

    auto ret = make_shared<ElfFile>(file_id, move(mapped_file));
    ret->__set_weak_this(ret);
    s_elf_file_map.put(file_id, ret.get());
    return ret;
}

ElfFile::ElfFile(FileId file_id, UniquePtr<Ext::MappedFile> file) : m_file_id(file_id), m_file(move(file)) {
    for (size_t i = 0; i < shdr_count(); i++) {
        auto& shdr = *shdr_at(i);
        if (shdr.sh_type == SHT_STRTAB && i != elf_header()->e_shstrndx) {
            m_string_table = &shdr;
        } else if (shdr.sh_type == SHT_SYMTAB) {
            m_symbol_table = &shdr;
        }
    }
}

ElfFile::~ElfFile() {
    s_elf_file_map.remove(m_file_id);
}

Maybe<String> ElfFile::lookup_symbol(uintptr_t addr) const {
    if (!m_string_table || !m_symbol_table) {
        return {};
    }

    size_t symbol_count = m_symbol_table->sh_size / sizeof(Elf64_Sym);
    auto* symbols = (const Elf64_Sym*) offset_in_memory(m_symbol_table->sh_offset);
    for (size_t i = 0; i < symbol_count; i++) {
        auto& sym = symbols[i];
        if (sym.st_value <= addr && addr < sym.st_value + sym.st_size) {
            return { string_at(sym.st_name) };
        }
    }

    return {};
}
