#include <dius/test/prelude.h>

constexpr void meta() {
    struct X {
        using Type = di::Method<X, void(di::This const&)>;
    };
    struct Y {};

    using Interface = di::meta::List<X, di::Method<Y, void(di::This &&)>>;

    static_assert(di::concepts::Interface<Interface>);
}

TESTC(any, meta)