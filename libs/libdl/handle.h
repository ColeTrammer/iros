#pragma once

#include <elf.h>
#include <errno.h>
#include <ext/mapped_file.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <liim/variant.h>
#include <sys/mman.h>

class MappedElfFile {
public:
    static Variant<UniquePtr<MappedElfFile>, String> create(const String& file) {
        auto file_mapping = Ext::MappedFile::create(file, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE);
        if (!file_mapping) {
            return String::format("failed to open `%s': %s", file.string(), strerror(errno));
        }

        auto* header = reinterpret_cast<Elf64_Ehdr*>(file_mapping->data());
        if (header->e_ident[EI_MAG0] != 0x7F || header->e_ident[EI_MAG1] != 'E' || header->e_ident[EI_MAG2] != 'L' ||
            header->e_ident[EI_MAG3] != 'F') {
            return String::format("`%s' is not an ELF file", file.string());
        }

        if (header->e_ident[EI_CLASS] != ELFCLASS64) {
            return String::format("`%s' is not ELF64", file.string());
        }

        if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
            return String::format("`%s' has incorrect byte order", file.string());
        }

        if (header->e_ident[EI_VERSION] != EV_CURRENT) {
            return String::format("`%s' has incorrect version", file.string());
        }

        if (header->e_ident[EI_OSABI] != ELFOSABI_SYSV || header->e_ident[EI_ABIVERSION] != 0) {
            return String::format("`%s' has incorrect ABI", file.string());
        }

        if (header->e_type != ET_DYN) {
            return String::format("`%s' is not a dynamic library", file.string());
        }

        return make_unique<MappedElfFile>(move(file_mapping));
    }

    MappedElfFile(UniquePtr<Ext::MappedFile> file_mapping) : m_file_mapping(move(file_mapping)) {}

    Elf64_Ehdr* elf_header() { return reinterpret_cast<Elf64_Ehdr*>(m_file_mapping->data()); }
    const Elf64_Ehdr* elf_header() const { return const_cast<MappedElfFile&>(*this).elf_header(); }

    Elf64_Shdr* section_table() {
        auto* elf_header = this->elf_header();
        return reinterpret_cast<Elf64_Shdr*>(m_file_mapping->data() + elf_header->e_shoff);
    }
    const Elf64_Shdr* section_table() const { return const_cast<MappedElfFile&>(*this).section_table(); }
    Elf64_Shdr* section_at(size_t index) { return &section_table()[index]; }
    const Elf64_Shdr* section_at(size_t index) const { return &section_table()[index]; }
    size_t section_count() const { return elf_header()->e_shnum; }

    Elf64_Phdr* program_header_table() {
        auto* elf_header = this->elf_header();
        return reinterpret_cast<Elf64_Phdr*>(m_file_mapping->data() + elf_header->e_phoff);
    }
    const Elf64_Phdr* program_header_table() const { return const_cast<MappedElfFile&>(*this).program_header_table(); }
    Elf64_Phdr* program_header_at(size_t index) { return &program_header_table()[index]; }
    const Elf64_Phdr* program_header_at(size_t index) const { return &program_header_table()[index]; }
    size_t program_header_count() const { return elf_header()->e_phnum; }

    const char* section_strings() const {
        auto* elf_header = this->elf_header();
        if (elf_header->e_shstrndx == 0) {
            return nullptr;
        }
        auto* string_table = section_at(elf_header->e_shstrndx);
        return reinterpret_cast<const char*>(m_file_mapping->data()) + string_table->sh_offset;
    }
    const char* section_string(size_t index) const {
        if (!section_strings()) {
            return nullptr;
        }
        return &section_strings()[index];
    }

    const char* strings() const {
        for (size_t i = 0; i < section_count(); i++) {
            auto* section = section_at(i);
            if (section->sh_type == SHT_STRTAB && strcmp(section_string(section->sh_name), ".strtab") == 0) {
                return reinterpret_cast<const char*>(m_file_mapping->data()) + section->sh_offset;
            }
        }
        return nullptr;
    }
    const char* string(size_t index) const {
        auto strings = this->strings();
        if (!strings) {
            return nullptr;
        }
        return &strings[index];
    }

    Elf64_Phdr* dynamic_program_header() {
        for (size_t i = 0; i < program_header_count(); i++) {
            auto* phdr = program_header_at(i);
            if (phdr->p_type == PT_DYNAMIC) {
                return phdr;
            }
        }
        return nullptr;
    }
    const Elf64_Phdr* dynamic_program_header() const { return const_cast<MappedElfFile&>(*this).dynamic_program_header(); }

    Elf64_Dyn* dynamic_table() {
        auto* phdr = this->dynamic_program_header();
        if (!phdr) {
            return nullptr;
        }
        return reinterpret_cast<Elf64_Dyn*>(m_file_mapping->data() + phdr->p_offset);
    }
    const Elf64_Dyn* dynamic_table() const { return const_cast<MappedElfFile&>(*this).dynamic_table(); }
    size_t dynamic_count() const {
        auto* phdr = this->dynamic_program_header();
        if (!phdr) {
            return 0;
        }
        return phdr->p_filesz / sizeof(Elf64_Dyn);
    }
    Elf64_Dyn* dynamic_entry(size_t i) { return &dynamic_table()[i]; }
    const Elf64_Dyn* dynamic_entry(size_t i) const { return &dynamic_table()[i]; }

    uint8_t* data() { return m_file_mapping->data(); }
    const uint8_t* data() const { return m_file_mapping->data(); }

private:
    UniquePtr<Ext::MappedFile> m_file_mapping;
};

class DynamicElfObject {
public:
    DynamicElfObject(const MappedElfFile& elf_file) : m_raw_data(elf_file.data()) {
        auto* dynamic_table = elf_file.dynamic_table();
        auto dynamic_count = elf_file.dynamic_count();
        for (size_t i = 0; i < dynamic_count; i++) {
            auto* entry = &dynamic_table[i];
            switch (entry->d_tag) {
                case DT_NULL:
                    return;
                case DT_HASH:
                    m_hash_table = entry->d_un.d_ptr;
                    break;
                case DT_STRTAB:
                    m_string_table = entry->d_un.d_ptr;
                    break;
                case DT_SYMTAB:
                    m_symbol_table = entry->d_un.d_ptr;
                    break;
                case DT_RELA:
                    m_rela_offset = entry->d_un.d_ptr;
                    break;
                case DT_RELASZ:
                    m_rela_size = entry->d_un.d_val;
                    break;
                case DT_RELAENT:
                    m_rela_entry_size = entry->d_un.d_val;
                    break;
                case DT_STRSZ:
                    m_string_table_size = entry->d_un.d_val;
                    break;
                case DT_SYMENT:
                    m_symbol_entry_size = entry->d_un.d_val;
                    break;
                case DT_INIT:
                    m_init_offset = entry->d_un.d_ptr;
                    break;
                case DT_FINI:
                    m_fini_offset = entry->d_un.d_ptr;
                    break;
                case DT_SONAME:
                    m_so_name_offset = entry->d_un.d_val;
                    break;
                case DT_TEXTREL:
                    break;
                case DT_RELACOUNT:
                    break;
                case DT_RELCOUNT:
                    break;
                default:
                    fprintf(stderr, "Unkown DT_* value %ld\n", entry->d_tag);
                    break;
            }
        }
    }

    const char* dynamic_strings() const { return reinterpret_cast<const char*>(m_raw_data + m_string_table); }
    const char* dynamic_string(size_t i) const { return &dynamic_strings()[i]; }

    size_t rela_count() const { return m_rela_size / m_rela_entry_size; }
    const Elf64_Rela* rela_table() const { return reinterpret_cast<const Elf64_Rela*>(m_raw_data + m_rela_offset); }
    const Elf64_Rela* rela_at(size_t i) const { return &rela_table()[i]; }

    const Elf64_Sym* symbol_table() const { return reinterpret_cast<const Elf64_Sym*>(m_raw_data + m_symbol_table); }
    const Elf64_Sym* symbol_at(size_t i) const { return &symbol_table()[i]; }
    const char* symbol_name(size_t i) const { return dynamic_string(symbol_at(i)->st_name); }

    const Elf64_Word* hash_table() const { return reinterpret_cast<const Elf64_Word*>(m_raw_data + m_hash_table); }
    const Elf64_Sym* lookup_symbol(const char* s) const {
        auto* hash_table = this->hash_table();
        auto nbucket = hash_table[0];
        auto nchain = hash_table[1];
        auto hashed_value = elf64_hash(s);
        auto bucket_index = hashed_value % nbucket;
        auto symbol_index = hash_table[2 + bucket_index];
        while (symbol_index != STN_UNDEF) {
            if (strcmp(symbol_name(symbol_index), s) == 0) {
                return symbol_at(symbol_index);
            }

            auto chain_index = 2 + nbucket + symbol_index;
            if (symbol_index >= nchain) {
                return nullptr;
            }
            symbol_index = hash_table[chain_index];
        }
        return nullptr;
    }

private:
    uintptr_t m_hash_table { 0 };
    uintptr_t m_string_table { 0 };
    uintptr_t m_symbol_table { 0 };
    uintptr_t m_init_offset { 0 };
    uintptr_t m_fini_offset { 0 };
    uintptr_t m_rela_offset { 0 };
    size_t m_rela_size { 0 };
    size_t m_rela_entry_size { 0 };
    size_t m_so_name_offset { 0 };
    size_t m_string_table_size { 0 };
    size_t m_symbol_entry_size { 0 };
    const uint8_t* m_raw_data { nullptr };
};

struct Handle {
    UniquePtr<MappedElfFile> elf_file;
    DynamicElfObject dynamic_object;
};
