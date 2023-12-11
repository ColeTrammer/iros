#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/concepts/container_of.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/prev.h>
#include <di/container/string/constant_string_interface.h>
#include <di/container/string/encoding.h>
#include <di/container/string/string_append.h>
#include <di/container/string/string_clear.h>
#include <di/container/string/string_push_back.h>
#include <di/container/string/string_to_vector_iterator.h>
#include <di/container/vector/vector_append_container.h>
#include <di/container/vector/vector_begin.h>
#include <di/container/vector/vector_clear.h>
#include <di/container/vector/vector_erase.h>
#include <di/container/vector/vector_size.h>
#include <di/container/view/concat.h>
#include <di/container/view/single.h>
#include <di/meta/core.h>
#include <di/platform/custom.h>
#include <di/util/create_in_place.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/error/meta/common_error.h>
#include <di/vocab/error/status_code_forward_declaration.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/invoke_as_fallible.h>
#include <di/vocab/expected/try_infallible.h>
#include <di/vocab/optional/lift_bool.h>

namespace di::container::string {
namespace detail {
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    template<typename U, typename R = U[1]>
    constexpr inline R empty_null_terminated_array = { U(0) };
}

template<typename Self, concepts::Encoding Enc>
class MutableStringInterface : public ConstantStringInterface<Self, Enc> {
private:
    using Encoding = Enc;
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;

    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

    template<concepts::InputContainer Con, typename... Args>
    requires(concepts::ContainerOf<Con, CodeUnit> && concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     encoding::AssumeValid, Args&&... args) {
        auto result = Self(di::forward<Args>(args)...);
        auto view = [&] {
            if constexpr (encoding::NullTerminated<Enc>) {
                return view::concat(di::forward<Con>(container), view::single(CodeUnit(0)));
            } else {
                return view::all(di::forward<Con>(container));
            }
        }();
        return invoke_as_fallible([&] {
                   return vector::append_container(result, util::move(view));
               }) % [&](auto&&...) {
            return util::move(result);
        } | try_infallible;
    }

    template<concepts::InputContainer Con, typename... Args>
    requires(!SameAs<CodeUnit, CodePoint> && concepts::ContainerOf<Con, CodeUnit> &&
             concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(di::forward<Args>(args)...);
        auto view = [&] {
            if constexpr (encoding::NullTerminated<Enc>) {
                return view::concat(di::forward<Con>(container), view::single(CodeUnit(0)));
            } else {
                return view::all(di::forward<Con>(container));
            }
        }();
        if constexpr (encoding::Universal<Enc>) {
            return invoke_as_fallible([&] {
                       return vector::append_container(result, util::move(view));
                   }) % [&](auto&&...) {
                return util::move(result);
            } | try_infallible;
        } else {
            return (invoke_as_fallible([&] {
                        return vector::append_container(result, util::move(view));
                    }) >>
                        [&] -> Expected<void, GenericCode> {
                       auto is_valid = encoding::validate(Enc {}, result.span());
                       if (!is_valid) {
                           return Unexpected(BasicError::InvalidArgument);
                       }
                       return {};
                   }) % [&] {
                return util::move(result);
            } | try_infallible;
        }
    }

    template<concepts::InputContainer Con, typename... Args>
    requires(concepts::ContainerOf<Con, CodePoint> && concepts::ConstructibleFrom<Self, Args...>)
    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<Self>, Con&& container,
                                     Args&&... args) {
        auto result = Self(di::forward<Args>(args)...);
        return invoke_as_fallible([&] {
                   return string::append(result, di::forward<Con>(container));
               }) % [&] {
            return util::move(result);
        } | try_infallible;
    }

public:
    constexpr void clear() { return string::clear(self()); }

    constexpr CodePoint& operator[](usize index)
    requires(encoding::Contiguous<Enc>)
    {
        DI_ASSERT(index < self().size());
        return vector::data(self())[index];
    }

    constexpr auto at(usize index) {
        return lift_bool(index < self().size()) % [&] {
            return self()[index];
        };
    }

    constexpr auto c_str() const
    requires(encoding::NullTerminated<Enc>)
    {
        if (self().empty()) {
            return detail::empty_null_terminated_array<CodeUnit>;
        }
        DI_ASSERT(self().size() < self().capacity());
        DI_ASSERT(string::data(self())[self().size()] == CodeUnit(0));
        return string::data(self());
    }

    constexpr auto push_back(CodePoint code_point) { return string::push_back(self(), code_point); }

    constexpr auto erase(Iterator first) {
        DI_ASSERT(first != this->end());
        return this->erase(first, next(first));
    }
    constexpr auto erase(Iterator first, Iterator last) -> Iterator {
        if (first == last) {
            return first;
        }

        if constexpr (encoding::NullTerminated<Enc>) {
            self().assume_size(vector::size(self()) + 1);
        }
        auto result = encoding::make_iterator(self().encoding(), as_const(self()).span(),
                                              vector::erase(self(), string::string_to_vector_iterator(self(), first),
                                                            string::string_to_vector_iterator(self(), last)) -
                                                  vector::begin(self()));
        if constexpr (encoding::NullTerminated<Enc>) {
            self().assume_size(vector::size(self()) - 1);
        }
        return result;
    }
    constexpr auto erase(usize offset, Optional<usize> count = {})
    requires(encoding::Contiguous<Enc>)
    {
        auto first = this->iterator_at_offset(offset);
        auto last = [&] -> Iterator {
            if (count) {
                return this->iterator_at_offset(offset + *count).value_or(this->end());
            }
            return this->end();
        }();
        if (!first.has_value()) {
            return this->end();
        }
        return erase(*first, last);
    }

    template<concepts::ContainerOf<CodePoint> Con>
    constexpr auto append(Con&& container) -> decltype(auto) {
        return invoke_as_fallible([&] {
                   return string::append(self(), di::forward<Con>(container));
               }) % [&] {
            return ref(self());
        } | try_infallible;
    }

    template<concepts::ContainerOf<CodePoint> Con>
    constexpr auto operator+=(Con&& container) -> decltype(auto) {
        return this->append(di::forward<Con>(container));
    }

    constexpr auto pop_back() {
        return this->back() % [&](auto value) {
            this->erase(prev(this->end()));
            return value;
        };
    }

    constexpr auto insert(Iterator it, CodePoint code_point) { return this->insert(it, view::single(code_point)); }
    constexpr auto insert(usize offset, CodePoint code_point)
    requires(encoding::Contiguous<Enc>)
    {
        auto it = this->iterator_at_offset(offset);
        DI_ASSERT(it.has_value());
        return this->insert(*it, view::single(code_point));
    }

    template<concepts::ContainerOf<CodePoint> Con>
    constexpr auto insert(Iterator it, Con&& container) {
        if constexpr (encoding::NullTerminated<Enc>) {
            return (invoke_as_fallible([&] {
                        return vector::insert_container(self(), string::string_to_vector_iterator(self(), it),
                                                        di::forward<Con>(container) |
                                                            view::transform(function::bind_front(
                                                                encoding::convert_to_code_units, self().encoding())) |
                                                            view::join);
                    }) >>
                    [&](auto result) {
                        return as_fallible(vector::emplace_back(self())) % [&](auto&) {
                            vector::pop_back(self());
                            return result;
                        };
                    }) %
                       [&](auto result) {
                           auto [first, last] = result;
                           return View {
                               encoding::make_iterator(self().encoding(), as_const(self()).span(),
                                                       first - vector::begin(self())),
                               encoding::make_iterator(self().encoding(), as_const(self()).span(),
                                                       last - vector::begin(self())),
                           };
                       } |
                   try_infallible;
        } else {
            return invoke_as_fallible([&] {
                       return vector::insert_container(self(), string::string_to_vector_iterator(self(), it),
                                                       di::forward<Con>(container) |
                                                           view::transform(function::bind_front(
                                                               encoding::convert_to_code_units, self().encoding())) |
                                                           view::join);
                   }) % [&](auto result) {
                auto [first, last] = result;
                return View {
                    encoding::make_iterator(self().encoding(), as_const(self()).span(), first - vector::begin(self())),
                    encoding::make_iterator(self().encoding(), as_const(self()).span(), last - vector::begin(self())),
                };
            } | try_infallible;
        }
    }

    template<concepts::ContainerOf<CodePoint> Con>
    requires(encoding::Contiguous<Enc>)
    constexpr auto insert(usize offset, Con&& container) {
        auto first = this->iterator_at_offset(offset);
        DI_ASSERT(first.has_value());
        return this->insert(*first, di::forward<Con>(container));
    }

    constexpr auto replace(Iterator first, Iterator last, CodePoint code_point) {
        return this->replace(first, last, view::single(code_point));
    }
    constexpr auto replace(usize offset, usize length, CodePoint code_point)
    requires(encoding::Contiguous<Enc>)
    {
        return this->replace(offset, length, view::single(code_point));
    }

    template<concepts::ContainerOf<CodePoint> Con>
    constexpr auto replace(Iterator first, Iterator last, Con&& container) {
        auto it = this->erase(first, last);
        return this->insert(it, di::forward<Con>(container));
    }

    template<concepts::ContainerOf<CodePoint> Con>
    requires(encoding::Contiguous<Enc>)
    constexpr auto replace(usize offset, usize length, Con&& container) {
        auto first = this->iterator_at_offset(offset);
        auto last = [&] -> Iterator {
            if (length) {
                return this->iterator_at_offset(offset + length).value_or(this->end());
            }
            return this->end();
        }();
        DI_ASSERT(first.has_value());
        return this->replace(*first, last, di::forward<Con>(container));
    }
};
}
