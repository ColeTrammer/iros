#pragma once

#include <liim/traits.h>

namespace App {

class ModelIndex {
public:
    ModelIndex() {}
    ModelIndex(int item, int field) : m_item(item), m_field(field) {}

    int item() const { return m_item; }
    int field() const { return m_field; }

    void clear() { m_item = m_field = -1; }
    bool valid() const { return m_item != -1 && m_field != -1; }

    bool operator==(const ModelIndex& other) const = default;

private:
    int m_item { -1 };
    int m_field { -1 };
};

}

namespace LIIM {
template<>
struct Traits<App::ModelIndex> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const App::ModelIndex& obj) { return Traits<int>::hash(obj.item()) + Traits<int>::hash(obj.field()); };
};
}
