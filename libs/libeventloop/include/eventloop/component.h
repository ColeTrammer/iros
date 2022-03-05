#include <eventloop/forward.h>

namespace App {
class Component {
public:
    virtual ~Component() {}

    Object& object() const { return m_object; }

    template<typename T>
    T& typed_object() const {
        return static_cast<T&>(m_object);
    }

    void attach() { did_attach(); }
    void detach() { did_detach(); }

protected:
    explicit Component(Object&);

    virtual void did_attach() {}
    virtual void did_detach() {}

private:
    Object& m_object;
};
}
