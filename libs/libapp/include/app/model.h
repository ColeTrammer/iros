#pragma once

#include <app/forward.h>
#include <app/model_data.h>
#include <app/model_index.h>
#include <eventloop/event.h>
#include <eventloop/object.h>

APP_EVENT(App, ModelUpdateEvent, Event, (), (), ())

namespace App {
class Model : public Object {
    APP_OBJECT(Model)

    APP_EMITS(Object, ModelUpdateEvent)

public:
    enum Role {
        Display,
        TextAlignment,
        Icon,
    };

    virtual int row_count() const = 0;
    virtual int col_count() const = 0;
    virtual ModelData data(const ModelIndex& index, int role) const = 0;
    virtual ModelData header_data(int col, int role) const = 0;

protected:
    void did_update();
};
}
