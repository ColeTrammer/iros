#include <di/meta/list/prelude.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_like.h>

namespace di::meta {
namespace detail {
    template<typename...>
    struct GetElementHelper;

    template<typename Tup, size_t index>
    struct GetElementHelper<Tup, SizeConstant<index>> : TypeConstant<TupleElement<Tup, index>> {};

    template<concepts::TupleLike Tup>
    struct GetElement {
        template<typename... Args>
        using Invoke = meta::Type<GetElementHelper<Tup, Args...>>;
    };
}

template<concepts::TupleLike Tup>
using TupleElements =
    meta::Transform<meta::AsList<meta::MakeIndexSequence<meta::TupleSize<Tup>>>, detail::GetElement<Tup>>;
}
