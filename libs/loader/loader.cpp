#include "loader.h"

#include <elf.h>

#define malloc loader_malloc
#define free   loader_free
#include <liim/vector.h>

class DynamicElfObject {
public:
    DynamicElfObject(Elf64_Dyn *dynamic_table, size_t dynamic_count, uint8_t *base) : m_raw_data(base) {
        for (size_t i = 0; i < dynamic_count; i++) {
            auto *entry = &dynamic_table[i];
            switch (entry->d_tag) {
                case DT_NULL:
                    return;
                case DT_NEEDED:
                    m_dependencies.add(entry->d_un.d_val);
                    break;
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
                    loader_log("Unkown DT_* value %ld", entry->d_tag);
                    break;
            }
        }
    }

    const char *dynamic_strings() const { return reinterpret_cast<const char *>(m_string_table); }
    const char *dynamic_string(size_t i) const { return &dynamic_strings()[i]; }

    size_t rela_count() const { return m_rela_size / m_rela_entry_size; }
    const Elf64_Rela *rela_table() const { return reinterpret_cast<const Elf64_Rela *>(m_rela_offset); }
    const Elf64_Rela *rela_at(size_t i) const { return &rela_table()[i]; }

    const Elf64_Sym *symbol_table() const { return reinterpret_cast<const Elf64_Sym *>(m_symbol_table); }
    const Elf64_Sym *symbol_at(size_t i) const { return &symbol_table()[i]; }
    const char *symbol_name(size_t i) const { return dynamic_string(symbol_at(i)->st_name); }

    const Elf64_Word *hash_table() const { return reinterpret_cast<const Elf64_Word *>(m_hash_table); }
    const Elf64_Sym *lookup_symbol(const char *s) const {
        auto *hash_table = this->hash_table();
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

    void process_relocations() {
        for (size_t i = 0; i < rela_count(); i++) {
            auto *rela = rela_at(i);
            auto type = ELF64_R_TYPE(rela->r_info);
            auto symbol_index = ELF64_R_SYM(rela->r_info);
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
                    loader_log("R_X86_64_64 for symbol %s\n", symbol_name(symbol_index));
                    break;
                case R_X86_64_GLOB_DAT:
                    loader_log("R_X86_64_GLOB_DATA for symbol %s\n", symbol_name(symbol_index));
                    break;
                case R_X86_64_RELATIVE: {
                    // B + A
                    auto B = reinterpret_cast<uintptr_t>(base());
                    auto A = rela->r_addend;
                    auto *addr = reinterpret_cast<uint64_t *>(base() + rela->r_offset);
                    *addr = B + A;
                    break;
                }
                default:
                    loader_log("Unkown relocation type %ld\n", type);
                    break;
            }
        }
    }

    uint8_t *base() { return m_raw_data; }
    const uint8_t *base() const { return m_raw_data; }

    const Vector<size_t> dependencies() const { return m_dependencies; }

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
    uint8_t *m_raw_data { nullptr };
    Vector<size_t> m_dependencies;
};

extern "C" {
void _entry(struct initial_process_info *info, int, char **, char **) {
    DynamicElfObject program(reinterpret_cast<Elf64_Dyn *>(info->program_dynamic_start), info->program_dynamic_size / sizeof(Elf64_Dyn),
                             reinterpret_cast<uint8_t *>(info->program_offset));
    for (auto d : program.dependencies()) {
        loader_log("NEEDS `%s'", program.dynamic_string(d));
    }
    _exit(42);
}
}
