#include <ext/json.h>

int main() {
    using namespace Ext::Json;

    Value v = Number(10);
    assert(v.is<Number>());

    auto o = Object();
    o.put<String>("hello", "world");

    assert(!o.get_as<Number>("w"));
    assert(!o.get_as<Number>("hello"));
    assert(*o.get_as<String>("hello") == "world");
    assert(o.get_or<Number>("q", 5) == 5);

    Value x = move(o);
    assert(x.is<Object>());

    auto a = Array();
    a.add(Value(String("asdf")));

    return 0;
}
