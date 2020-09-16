#pragma once

#include <app/model_data.h>
#include <app/model_index.h>
#include <app/object.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace App {

class ModelClient;

class Model : public Object {
    APP_OBJECT(Model)

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

    void register_client(ModelClient* view);
    void unregister_client(ModelClient* view);

protected:
    void did_update();

private:
    Vector<ModelClient*> m_clients;
};

}
