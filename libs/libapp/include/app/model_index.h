#pragma once

#include <liim/traits.h>

namespace App {

class ModelIndex {
public:
    ModelIndex() {}
    ModelIndex(int r, int c) : m_row(r), m_col(c) {}

    int row() const { return m_row; }
    int col() const { return m_col; }

    void clear() { m_row = m_col = -1; }
    bool valid() const { return m_row != -1 && m_col != -1; }

    bool operator==(const ModelIndex& other) const { return this->row() == other.row() && this->col() == other.col(); }
    bool operator!=(const ModelIndex& other) const { return !(*this == other); }

private:
    int m_row { -1 };
    int m_col { -1 };
};

}

namespace LIIM {

template<>
struct Traits<App::ModelIndex> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const App::ModelIndex& obj) { return Traits<int>::hash(obj.row()) + Traits<int>::hash(obj.col()); };
};

}
