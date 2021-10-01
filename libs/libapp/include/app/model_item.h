#pragma once

#include <app/forward.h>
#include <app/model_item_info.h>
#include <liim/pointers.h>
#include <liim/vector.h>

namespace App {
class ModelItem {
public:
    ModelItem() = default;
    virtual ~ModelItem() = default;

    virtual ModelItemInfo info(int, int) const { return {}; }

    int item_count() const { return m_children.size(); }

    ModelItem* model_item_at(int index) { return m_children[index].get(); }

    template<typename T>
    T& typed_item(int index) {
        return static_cast<T&>(*m_children[index].get());
    }

    void add_child(UniquePtr<ModelItem> item) { m_children.add(move(item)); }
    void insert_child(UniquePtr<ModelItem> item, int index) { m_children.insert(move(item), index); }
    void remove_child(int index) { m_children.remove(index); }
    void clear_children() { m_children.clear(); }

private:
    Vector<UniquePtr<ModelItem>> m_children;
};
}
