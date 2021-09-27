#pragma once

#include <app/forward.h>
#include <app/model_index.h>
#include <app/model_item_info.h>
#include <eventloop/event.h>
#include <eventloop/object.h>

APP_EVENT(App, ModelUpdateEvent, Event, (), (), ())

namespace App {
class Model : public Object {
    APP_OBJECT(Model)

    APP_EMITS(Object, ModelUpdateEvent)

public:
    struct ModelDimensions {
        int item_count { 0 };
        int field_count { 1 };
    };

    virtual ModelDimensions dimensions() const = 0;
    virtual ModelItemInfo item_info(const ModelIndex& index, int request) const = 0;
    virtual ModelItemInfo header_info(int field, int request) const = 0;

protected:
    void did_update();
};
}
