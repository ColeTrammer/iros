#pragma once

#include <di/container/concepts/indirectly_readable.h>
#include <di/container/meta/iterator_common_reference.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_value.h>
#include <di/function/invoke.h>
#include <di/meta/operations.h>

namespace di::concepts {
template<typename F, typename It>
concept IndirectlyUnaryInvocable =
    IndirectlyReadable<It> && CopyConstructible<F> && Invocable<F&, meta::IteratorValue<It>&> &&
    Invocable<F&, meta::IteratorReference<It>> && Invocable<F&, meta::IteratorCommonReference<It>> &&
    CommonReferenceWith<meta::InvokeResult<F&, meta::IteratorValue<It>&>,
                        meta::InvokeResult<F&, meta::IteratorReference<It>>>;
}
