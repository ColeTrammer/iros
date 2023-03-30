#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/convertible_to.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/unexpected.h>

namespace di::concepts::detail {
template<typename T, typename E, typename U, typename G>
concept ExpectedCanConvertConstructor =
    (!ConstructibleFrom<T, vocab::Expected<U, G>> && !ConstructibleFrom<T, vocab::Expected<U, G> const> &&
     !ConstructibleFrom<T, vocab::Expected<U, G>&> && !ConstructibleFrom<T, vocab::Expected<U, G> const&> &&
     !ConvertibleTo<vocab::Expected<U, G>, T> && !ConvertibleTo<vocab::Expected<U, G> const, T> &&
     !ConvertibleTo<vocab::Expected<U, G>&, T> && !ConvertibleTo<vocab::Expected<U, G> const&, T> &&
     (concepts::LanguageVoid<E> || (!ConstructibleFrom<vocab::Unexpected<E>, vocab::Expected<U, G>> &&
                                    !ConstructibleFrom<vocab::Unexpected<E>, vocab::Expected<U, G> const> &&
                                    !ConstructibleFrom<vocab::Unexpected<E>, vocab::Expected<U, G>&> &&
                                    !ConstructibleFrom<vocab::Unexpected<E>, vocab::Expected<U, G> const&>) ));
}
