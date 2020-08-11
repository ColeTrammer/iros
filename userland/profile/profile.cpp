#include <ext/mapped_file.h>
#include <liim/hash_map.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <sys/types.h>

#include "elf_file.h"
#include "profile.h"

#define PEV_AT_OFFSET(buffer, o) ((const profile_event*) (((uint8_t*) (buffer)) + (o)))

class MemoryMap {
public:
    struct MemoryObject {
        uintptr_t start;
        uintptr_t end;
        SharedPtr<ElfFile> file;
    };

    void add(MemoryObject object) { m_objects.add(move(object)); }
    void clear() { m_objects.clear(); }

    const MemoryObject* find_by_addr(uintptr_t addr) const {
        for (auto& object : m_objects) {
            if (object.start <= addr && addr < object.end) {
                return &object;
            }
        }
        return nullptr;
    }

private:
    Vector<MemoryObject> m_objects;
};

UniquePtr<Profile> Profile::create(const String& path) {
    auto file = Ext::MappedFile::create(path, PROT_READ, MAP_SHARED);
    if (!file) {
        return nullptr;
    }

    MemoryMap current_memory_map;
    auto process_stack_trace = [&](const profile_event_stack_trace* ev) {
        for (size_t i = 0; i < ev->count; i++) {
            auto addr = ev->frames[i];

            String symbol_name = "??";
            auto* memory_object = current_memory_map.find_by_addr(addr);
            if (memory_object && memory_object->file) {
                uintptr_t offset = addr;
                if (memory_object->file->relocatable()) {
                    offset -= memory_object->start;
                }
                auto result = memory_object->file->lookup_symbol(offset);
                if (result.has_value()) {
                    symbol_name = result.value();
                }
            }
            fprintf(stderr, "Symbol: [ %#.16lX, %s ]\n", addr, symbol_name.string());
        }
        fprintf(stderr, "\n");
    };

    auto process_memory_map = [&](const profile_event_memory_map* ev) {
        current_memory_map.clear();
        for (size_t i = 0; i < ev->count; i++) {
            auto& raw_object = ev->mem[i];
            current_memory_map.add(
                { raw_object.start, raw_object.end, ElfFile::find_or_create({ raw_object.inode_id, raw_object.fs_id }) });
        }
    };

    size_t offset = 0;
    while (offset < file->size()) {
        auto* pev = PEV_AT_OFFSET(file->data(), offset);
        switch (pev->type) {
            case PEV_STACK_TRACE: {
                auto* ev = (const profile_event_stack_trace*) pev;
                process_stack_trace(ev);
                offset += PEV_STACK_TRACE_SIZE(ev);
                break;
            }
            case PEV_MEMORY_MAP: {
                auto* ev = (const profile_event_memory_map*) pev;
                process_memory_map(ev);
                offset += PEV_MEMORY_MAP_SIZE(ev);
                break;
            }
            default:
                fprintf(stderr, "profile: unknown profile event: %d\n", pev->type);
                return nullptr;
        }
    }

    return nullptr;
}

void view_profile(const String& path) {
    auto profile = Profile::create(path);
    if (!profile) {
        fprintf(stderr, "profile: failed to read profile file `%s'\n", path.string());
        exit(1);
    }
}
