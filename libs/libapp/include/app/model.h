#pragma once

#include <app/forward.h>
#include <app/model_index.h>
#include <app/model_item.h>
#include <app/model_item_info.h>
#include <eventloop/event.h>
#include <eventloop/object.h>

APP_EVENT(App, ModelUpdateEvent, Event, (), (), ())

namespace App {
class Model : public Object {
    APP_OBJECT(Model)

    APP_EMITS(Object, ModelUpdateEvent)

public:
    virtual int field_count() const = 0;
    virtual ModelItemInfo header_info(int field, int request) const = 0;

    ModelItem* model_item_root() { return m_root.get(); }
    const ModelItem* model_item_root() const { return m_root.get(); }

    template<typename T>
    T* typed_root() {
        return static_cast<T*>(m_root.get());
    }

    template<typename T>
    const T* typed_root() const {
        return static_cast<const T*>(m_root.get());
    }

    template<typename T, typename... Args>
    T& add_child(ModelItem& parent, Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        parent.add_child(move(child));
        did_update();
        return ret;
    }

    template<typename T, typename... Args>
    T& insert_child(ModelItem& parent, int index, Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        parent.insert_child(index, move(child));
        did_update();
        return ret;
    }

    void remove_child(ModelItem& parent, int index) {
        parent.remove_child(index);
        did_update();
    }

    void clear_children(ModelItem& parent) {
        parent.clear_children();
        did_update();
    }

protected:
    Model();

    void did_update();

    void set_root(UniquePtr<ModelItem> root);

private:
    UniquePtr<ModelItem> m_root;
};
}
