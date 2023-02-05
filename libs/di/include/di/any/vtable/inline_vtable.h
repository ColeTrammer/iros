#pragma once

#include <di/any/concepts/any_storable.h>
#include <di/any/concepts/interface.h>
#include <di/any/meta/method_erased_signature.h>
#include <di/any/vtable/erased_call.h>
#include <di/util/get.h>
#include <di/vocab/tuple/prelude.h>

namespace di::any {
struct InlineVTable {
    template<typename Interface>
    class Invoke;

    template<typename... Methods>
    requires(concepts::Conjunction<concepts::Method<meta::Type<Methods>>...>)
    class Invoke<meta::List<Methods...>> {
        static_assert(sizeof...(Methods) > 0, "Cannot create an InlineVTable with 0 methods.");

    private:
        using Storage = Tuple<meta::MethodErasedSignature<meta::Type<Methods>>*...>;

    public:
        template<typename Storage, concepts::AnyStorable<Storage> T>
        constexpr static Invoke create_for() {
            return Invoke(make_tuple(detail::ErasedCallImpl<meta::Type<Methods>, Storage, T>::call...));
        }

        constexpr Invoke() { reset(); }

        constexpr bool empty() const { return util::get<0>(m_storage) == nullptr; }
        constexpr void reset() { util::get<0>(m_storage) = nullptr; }

        template<concepts::OneOf<meta::Type<Methods>...> Method>
        constexpr auto operator[](Method) const {
            constexpr auto index = meta::Lookup<Method, meta::List<meta::Type<Methods>...>>;
            return util::get<index>(m_storage);
        }

    private:
        constexpr Invoke(Storage storage) : m_storage(storage) {}

        Storage m_storage;
    };
};
}