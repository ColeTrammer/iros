#include <elf.h>
#include <ext/mapped_file.h>
#include <liim/hash_map.h>
#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <sys/types.h>

struct FileId {
    ino_t inode_id;
    dev_t fs_id;

    bool operator==(const FileId& other) const { return this->inode_id == other.inode_id && this->fs_id == other.fs_id; }
    bool operator!=(const FileId& other) const { return !(*this == other); }
};

namespace LIIM {
template<>
struct Traits<FileId> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const FileId& id) { return id.inode_id + id.fs_id; }
};
}

class ElfFile {
public:
    static SharedPtr<ElfFile> find_or_create(FileId id);
    static SharedPtr<ElfFile> create(FileId id, const String& path);

    ElfFile(FileId file_id, UniquePtr<Ext::MappedFile> file);
    ~ElfFile();

    SharedPtr<ElfFile> shared_from_this() { return m_weak_this.lock(); }
    SharedPtr<const ElfFile> shared_from_this() const { return m_weak_this.lock(); }

    WeakPtr<ElfFile> weak_from_this() { return m_weak_this; }
    WeakPtr<const ElfFile> weak_from_this() const { return m_weak_this; }

    void __set_weak_this(WeakPtr<ElfFile> weak_this) {
        assert(m_weak_this.expired());
        m_weak_this = move(weak_this);
    }

    const uint8_t* offset_in_memory(size_t offset) const { return m_file->data() + offset; }
    const uint8_t* section_in_memory(const Elf64_Shdr* section) const { return offset_in_memory(section->sh_offset); }

    const char* string_at(const Elf64_Shdr* str_section, size_t i) const {
        if (!m_string_table) {
            return nullptr;
        }
        return (const char*) section_in_memory(str_section) + i;
    }
    const char* string_at(size_t i) const { return string_at(m_string_table, i); }

    const Elf64_Ehdr* elf_header() const { return (const Elf64_Ehdr*) m_file->data(); }
    size_t phdr_size() const { return elf_header()->e_phentsize; }
    size_t phdr_count() const { return elf_header()->e_phnum; }
    const Elf64_Phdr* phdr_at(size_t i) { return (const Elf64_Phdr*) offset_in_memory(elf_header()->e_phoff + shdr_size() * i); }

    size_t shdr_size() const { return elf_header()->e_shentsize; }
    size_t shdr_count() const { return elf_header()->e_shnum; }
    const Elf64_Shdr* shdr_at(size_t i) { return (const Elf64_Shdr*) offset_in_memory(elf_header()->e_shoff + shdr_size() * i); }

    Maybe<String> lookup_symbol(uintptr_t offset) const;
    bool relocatable() const { return elf_header()->e_type == ET_DYN; }

private:
    FileId m_file_id { 0, 0 };
    UniquePtr<Ext::MappedFile> m_file;
    const Elf64_Shdr* m_string_table { nullptr };
    const Elf64_Shdr* m_symbol_table { nullptr };
    mutable WeakPtr<ElfFile> m_weak_this;
};
