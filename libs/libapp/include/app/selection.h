#pragma once

#include <app/model_index.h>
#include <liim/hash_set.h>

namespace App {
class Selection {
public:
    bool present(const ModelIndex& index) const { return !!m_indexes.get(index); }
    void toggle(const ModelIndex& index) { m_indexes.toggle(index); }
    void add(ModelIndex index) { m_indexes.put(move(index)); }
    void remove(const ModelIndex& index) { m_indexes.remove(index); }

    void clear() { m_indexes.clear(); }
    bool empty() const { return m_indexes.size() == 0; }

    const ModelIndex& first() const {
        ModelIndex* ret = nullptr;
        m_indexes.for_each([&](auto& i) {
            ret = &i;
        });
        return *ret;
    }

private:
    HashSet<ModelIndex> m_indexes;
};
}
