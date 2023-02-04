#include <dius/test/prelude.h>

constexpr void meta() {
    struct X {
        using Type = di::Method<X, void(di::This const&)>;
    };
    struct Y {};

    using Interface = di::meta::List<X, di::Method<Y, void(di::This &&)>>;

    static_assert(di::concepts::Interface<Interface>);

    using S = di::meta::MethodErasedSignature<di::meta::Type<X>>;
    static_assert(di::SameAs<S, void(void*)>);

    static_assert(di::SameAs<di::meta::MethodErasedSignature<di::Method<Y, i32(i32, di::String const&, di::This&&)>>,
                             i32(i32, di::String const&, void*)>);
}

TESTC(any, meta)