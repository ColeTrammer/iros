#include <ext/mapped_file.h>
#include <liim/container/container.h>
#include <liim/hash_map.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/iros.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "elf_file.h"
#include "profile.h"

// #define PROFILE_DEBUG

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

    Vector<MemoryObject>& objects() { return m_objects; }
    const Vector<MemoryObject>& objects() const { return m_objects; }

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

UniquePtr<Profile> Profile::create(const String& path, bool invert_profile) {
    auto file = Ext::try_map_file(path, PROT_READ, MAP_SHARED);
    if (!file) {
        return nullptr;
    }

    size_t exec_name_length = *((size_t*) file->data());
    auto exec_name = String((char*) (file->data() + sizeof(size_t)));
    SharedPtr<ElfFile> executable = ElfFile::create(exec_name);
    SharedPtr<ElfFile> kernel_object = ElfFile::create("/boot/kernel");
    ProfileNode root(exec_name, exec_name, 0);

    MemoryMap current_memory_map;
    auto add_kernel_object = [&] {
#ifdef __x86_64__
        current_memory_map.add({ 0xFFFFFF0000000000, 0xFFFFFFFFFFFFFFFF, kernel_object });
#else
        current_memory_map.add({ 0xC0000000, 0xFFFFFFFF, kernel_object });
#endif
    };
    add_kernel_object();

    auto process_stack_trace = [&](const profile_event_stack_trace* ev) {
        if (ev->count == 0) {
            return;
        }

        auto* current_node = &root;
        for (size_t i = invert_profile ? 1 : ev->count; invert_profile ? i <= ev->count : i > 0; invert_profile ? i++ : i--) {
            auto addr = ev->frames[i - 1];

            String symbol_name = "??";
            String object_path = "??";
            auto* memory_object = current_memory_map.find_by_addr(addr);
            if (memory_object && memory_object->file) {
                uintptr_t offset = addr;
                if (memory_object->file->relocatable()) {
                    offset -= memory_object->start;
                }
                auto result = memory_object->file->lookup_symbol(offset);
                if (result.has_value()) {
                    symbol_name = result.value().name;
                    addr -= result.value().offset;
                    object_path = memory_object->file->path();
                }
            }

            if (!invert_profile || i != 2) {
                current_node->bump_total_count();
            }

            current_node = current_node->link(symbol_name, object_path, addr);
            if (invert_profile && i == 1) {
                current_node->bump_self_count();
            }

#ifdef PROFILE_DEBUG
            printf("Symbol: [ %#.16lX, %s ]\n", addr, symbol_name.string());
#endif /* PROFILE_DEBUG */
        }

        if (!invert_profile) {
            current_node->bump_self_count();
        } else {
            current_node->bump_total_count();
        }

#ifdef PROFILE_DEBUG
        printf("\n");
#endif /* PROFILE_DEBUG */
    };

    auto process_memory_map = [&](const profile_event_memory_map* ev) {
        MemoryMap new_memory_map;
        for (size_t i = 0; i < ev->count; i++) {
            auto& raw_object = ev->mem[i];
            new_memory_map.add({ raw_object.start, raw_object.end, ElfFile::find_or_create({ raw_object.inode_id, raw_object.fs_id }) });
        }
        current_memory_map.objects() = move(new_memory_map.objects());
        add_kernel_object();
    };

    size_t offset = sizeof(size_t) + exec_name_length;
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

#ifdef PROFILE_DEBUG
    for (auto& object : current_memory_map.objects()) {
        fprintf(stderr, "Object: [ %#.16lX, %#.16lX, %s ]\n", object.start, object.end, object.file ? object.file->path().string() : "??");
    }
#endif /* PROFILE_DEBUG */

    root.sort();

    return make_unique<Profile>(move(root));
}

ProfileNode* ProfileNode::link(const String& name, const String& object_path, uintptr_t address) {
    auto* node = m_nodes.first_match([&](auto& n) {
        return n.name() == name && n.address() == address && n.object_path() == object_path;
    });

    if (!node) {
        m_nodes.add(ProfileNode(name, object_path, address));
        node = &m_nodes.last();
    }

    return node;
}

void ProfileNode::dump(int level) const {
    printf("%*s ( %lu / %lu )\n", static_cast<int>(m_name.size()) + level, m_name.string(), m_self_count, m_total_count);
    for (auto& child : m_nodes) {
        child.dump(level + 1);
    }
}

void ProfileNode::sort() {
    Alg::sort(m_nodes, CompareThreeWayBackwards {}, &ProfileNode::total_count);

    for (auto& node : m_nodes) {
        node.sort();
    }
}

void Profile::dump() const {
    m_root.dump(0);
}

void view_profile(const String& path, bool invert_profile) {
    auto profile = Profile::create(path, invert_profile);
    if (!profile) {
        fprintf(stderr, "profile: failed to read profile file `%s'\n", path.string());
        exit(1);
    }

    profile->dump();
}
