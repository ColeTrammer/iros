#pragma once

#include <app/model_data.h>
#include <app/object.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace App {

class ModelIndex {
public:
    ModelIndex(int r, int c) : m_row(r), m_col(c) {}

    int row() const { return m_row; }
    int col() const { return m_col; }

private:
    int m_row { -1 };
    int m_col { -1 };
};

class Model : public Object {
    APP_OBJECT(Model)

public:
    virtual ~Model() {}

    virtual int row_count() const = 0;
    virtual int col_count() const = 0;
    virtual ModelData data(const ModelIndex& index) const = 0;
    virtual ModelData header_data(int col) const = 0;

protected:
    Model() {}
};

}
