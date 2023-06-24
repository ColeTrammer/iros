#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>

namespace di::meta {
template<bool is_const, typename T>
using MaybeConst = Conditional<is_const, T const, T>;
}

namespace di::concepts {
template<typename T, typename U>
concept NotSameAs = (!SameAs<T, U>);

template<typename T, typename U>
concept RemoveCVSameAs = SameAs<meta::RemoveCV<T>, meta::RemoveCV<U>>;

template<typename T, typename U>
concept RemoveCVRefSameAs = SameAs<meta::RemoveCVRef<T>, meta::RemoveCVRef<U>>;

template<typename T, typename U>
concept Like = SameAs<meta::RemoveCVRef<T>, U>;

template<typename T, typename U>
concept NotLike = (!Like<T, U>);

template<typename T, typename U>
concept LikeDerivedFrom = DerivedFrom<meta::RemoveCVRef<T>, U>;

template<typename T, typename U>
concept NotLikeDerivedFrom = (!LikeDerivedFrom<T, U>);

template<typename T>
concept RemoveCVRefConstructible = ConstructibleFrom<meta::RemoveCVRef<T>, T>;

template<typename T>
concept RemoveCVRefConvertible = ConvertibleTo<T, meta::RemoveCVRef<T>>;

template<typename T, typename U>
concept SameQualifiersAs = SameAs<meta::Like<T, int>, meta::Like<U, int>>;

template<typename From, typename To>
concept DecaysTo = SameAs<meta::Decay<From>, To>;

template<typename From, typename To>
concept NotDecaysTo = (!DecaysTo<From, To>);

template<typename T, typename U>
concept DecayDerivedFrom = DerivedFrom<meta::Decay<T>, U>;

template<typename T, typename U>
concept NotDecayDerivedFrom = (!DecayDerivedFrom<T, U>);

template<typename T, typename U>
concept DecaySameAs = SameAs<meta::Decay<T>, meta::Decay<U>>;

template<typename T>
concept ClassType = DecaysTo<T, T> && Class<T>;

template<typename T>
concept DecayConstructible = ConstructibleFrom<meta::Decay<T>, T>;

template<typename T>
concept DecayConvertible = ConvertibleTo<T, meta::Decay<T>>;

template<typename T>
concept MovableValue = DecayConstructible<T> && MoveConstructible<meta::Decay<T>>;
}
