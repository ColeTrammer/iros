#pragma once

#include <app/model_item.h>
#include <liim/hash_set.h>

namespace App {
class Selection {
public:
    bool present(ModelItem& item) const { return !!m_items.get(&item); }
    void toggle(ModelItem& item) { m_items.toggle(&item); }
    void add(ModelItem& item) { m_items.put(&item); }
    void remove(ModelItem& item) { m_items.remove(&item); }

    void clear() { m_items.clear(); }
    bool empty() const { return m_items.size() == 0; }

    ModelItem& first() const {
        ModelItem* ret = nullptr;
        m_items.for_each([&](auto& i) {
            ret = i;
        });
        return *ret;
    }

private:
    HashSet<ModelItem*> m_items;
};
}
