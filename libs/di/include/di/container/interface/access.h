#pragma once

#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/common_container.h>
#include <di/container/concepts/forward_container.h>
#include <di/container/concepts/prelude.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/interface/empty.h>
#include <di/container/interface/end.h>
#include <di/container/interface/ssize.h>
#include <di/container/iterator/prev.h>
#include <di/container/meta/container_reference.h>
#include <di/container/meta/container_ssize_type.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/function/curry_back.h>
#include <di/function/pipeable.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>

namespace di::container {
namespace detail {
    struct FrontFunction;

    template<typename T>
    concept CustomFront = concepts::Container<T> && concepts::TagInvocable<FrontFunction, T&>;

    template<typename T>
    concept MemberFront = concepts::Container<T> && requires(T&& container) {
        { container.front() } -> concepts::ImplicitlyConvertibleTo<Optional<meta::ContainerReference<T>>>;
    };

    template<typename T>
    concept BeginFront = concepts::ForwardContainer<T>;

    struct FrontFunction : function::pipeline::EnablePipeline {
        template<typename T>
        requires(CustomFront<T> || MemberFront<T> || BeginFront<T>)
        constexpr auto operator()(T&& container) const -> Optional<meta::ContainerReference<T>> {
            if constexpr (CustomFront<T>) {
                static_assert(SameAs<Optional<meta::ContainerReference<T>>, meta::TagInvokeResult<FrontFunction, T&>>,
                              "di::front() customizations must return Optional<meta::ContainerReference<T>>");
                return function::tag_invoke(*this, container);
            } else if constexpr (MemberFront<T>) {
                return container.front();
            } else {
                if (empty(container)) {
                    return nullopt;
                }
                return *begin(container);
            }
        }
    };
}

constexpr inline auto front = detail::FrontFunction {};

namespace detail {
    struct BackFunction;

    template<typename T>
    concept CustomBack = concepts::Container<T> && concepts::TagInvocable<BackFunction, T&>;

    template<typename T>
    concept MemberBack = concepts::Container<T> && requires(T&& container) {
        { container.back() } -> concepts::ImplicitlyConvertibleTo<Optional<meta::ContainerReference<T>>>;
    };

    template<typename T>
    concept RBeginBack = concepts::BidirectionalContainer<T> && concepts::CommonContainer<T>;

    struct BackFunction : function::pipeline::EnablePipeline {
        template<typename T>
        requires(CustomBack<T> || MemberBack<T> || RBeginBack<T>)
        constexpr auto operator()(T&& container) const -> Optional<meta::ContainerReference<T>> {
            if constexpr (CustomBack<T>) {
                static_assert(SameAs<Optional<meta::ContainerReference<T>>, meta::TagInvokeResult<BackFunction, T&>>,
                              "di::back() customizations must return Optional<meta::ContainerReference<T>>");
                return function::tag_invoke(*this, container);
            } else if constexpr (MemberBack<T>) {
                return container.back();
            } else {
                if (empty(container)) {
                    return nullopt;
                }
                return *prev(end(container));
            }
        }
    };
}

constexpr inline auto back = detail::BackFunction {};

namespace detail {
    struct AtFunction;

    template<typename T, typename K>
    concept CustomAt = concepts::Container<T> && concepts::TagInvocable<AtFunction, T&, K&&>;

    template<typename T, typename K>
    concept MemberAt = concepts::Container<T> && requires(T&& container, K&& key) {
        { container.at(di::forward<K>(key)) } -> concepts::Optional;
    };

    template<typename T, typename K>
    concept IndexAt = concepts::Container<T> && concepts::ConstructibleFrom<meta::IteratorSSizeType<T>, K> &&
                      requires(T& container, meta::IteratorSSizeType<T> index) {
                          { container[index] } -> SameAs<meta::ContainerReference<T>>;
                      };

    template<typename T, typename K>
    concept IteratorAt =
        concepts::RandomAccessContainer<T> && concepts::ConstructibleFrom<meta::IteratorSSizeType<T>, K>;

    struct AtFunction : function::pipeline::EnablePipeline {
        template<typename T, typename K>
        requires(CustomAt<T, K> || MemberAt<T, K> || IndexAt<T, K> || IteratorAt<T, K>)
        constexpr auto operator()(T&& container, K&& key) const {
            using SSizeType = meta::ContainerSSizeType<T>;
            if constexpr (CustomAt<T, K>) {
                static_assert(concepts::Optional<meta::TagInvokeResult<AtFunction, T&, K&&>>,
                              "di::at() customizations must return an Optional");
                return function::tag_invoke(*this, container, di::forward<K>(key));
            } else if constexpr (MemberAt<T, K>) {
                return container.at(di::forward<K>(key));
            } else {
                using R = Optional<meta::ContainerReference<T>>;

                auto const size = ssize(container);
                auto const index = SSizeType(di::forward<K>(key));
                if (index < 0 || index >= size) {
                    return R(nullopt);
                }

                if constexpr (IndexAt<T, K>) {
                    return R(container[index]);
                } else {
                    return R(begin(container)[index]);
                }
            }
        }
    };
}

constexpr inline auto at = di::curry_back(detail::AtFunction {}, c_<2zu>);

namespace detail {
    struct FrontUncheckedFunction : function::pipeline::EnablePipeline {
        template<typename T>
        constexpr auto operator()(T&& container) const -> decltype(auto)
        requires(requires { front(di::forward<T>(container)); })
        {
            return *front(container);
        }
    };

    struct BackUncheckedFunction : function::pipeline::EnablePipeline {
        template<typename T>
        constexpr auto operator()(T&& container) const -> decltype(auto)
        requires(requires { back(di::forward<T>(container)); })
        {
            return *back(container);
        }
    };

    struct AtUncheckedFunction {
        template<typename T, typename K>
        constexpr auto operator()(T&& container, K&& key) const -> decltype(auto)
        requires(requires { at(di::forward<T>(container), di::forward<K>(key)); })
        {
            return *at(container, di::forward<K>(key));
        }
    };
}

constexpr inline auto front_unchecked = detail::FrontUncheckedFunction {};
constexpr inline auto back_unchecked = detail::BackUncheckedFunction {};
constexpr inline auto at_unchecked = di::curry_back(detail::AtUncheckedFunction {}, c_<2zu>);
}

namespace di {
using container::at;
using container::at_unchecked;
using container::back;
using container::back_unchecked;
using container::front;
using container::front_unchecked;
}
