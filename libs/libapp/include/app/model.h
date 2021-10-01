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

protected:
    Model();

    void did_update();

    void set_root(UniquePtr<ModelItem> root);

private:
    UniquePtr<ModelItem> m_root;
};
}
