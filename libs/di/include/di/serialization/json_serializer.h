#pragma once

#include <di/concepts/always_false.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/integral.h>
#include <di/container/action/sequence.h>
#include <di/container/string/fixed_string_to_utf8_string_view.h>
#include <di/container/string/string_impl.h>
#include <di/container/string/string_view.h>
#include <di/container/string/utf8_encoding.h>
#include <di/container/vector/static_vector.h>
#include <di/function/invoke.h>
#include <di/io/interface/writer.h>
#include <di/io/prelude.h>
#include <di/io/write_exactly.h>
#include <di/reflect/reflect.h>
#include <di/util/declval.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/util/scope_value_change.h>
#include <di/vocab/error/result.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_for_each.h>
#include <di/vocab/tuple/tuple_sequence.h>

namespace di::serialization {
class JsonSerializerConfig {
public:
    JsonSerializerConfig() = default;

    constexpr JsonSerializerConfig pretty() const {
        auto config = *this;
        config.m_pretty = true;
        return config;
    }

    constexpr JsonSerializerConfig indent_width(int width) const {
        auto config = *this;
        config.m_indent_width = width;
        return config;
    }

    constexpr bool is_pretty() const { return m_pretty; }
    constexpr int indent_width() const { return m_indent_width; }

private:
    bool m_pretty { false };
    int m_indent_width { 4 };
};

template<concepts::Impl<Writer> Writer>
class JsonSerializer {
private:
    enum class State {
        First,
        Value,
        Normal,
    };

    class ObjectSerializerProxy {
    public:
        constexpr explicit ObjectSerializerProxy(JsonSerializer& serializer) : m_serializer(serializer) {}

        constexpr meta::WriterResult<void, Writer> serialize_string(container::StringView key,
                                                                    container::StringView view) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_string(view);
        }

        constexpr meta::WriterResult<void, Writer> serialize_number(container::StringView key,
                                                                    concepts::Integral auto number) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_number(number);
        }

        template<concepts::InvocableTo<meta::WriterResult<void, Writer>, JsonSerializer&> F>
        constexpr meta::WriterResult<void, Writer> serialize_array(container::StringView key, F&& function) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_array(util::forward<F>(function));
        }

        template<concepts::InvocableTo<meta::WriterResult<void, Writer>, ObjectSerializerProxy&> F>
        constexpr meta::WriterResult<void, Writer> serialize_object(container::StringView key, F&& function) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize_object(util::forward<F>(function));
        }

        template<typename T>
        requires(requires { util::declval<JsonSerializer&>().serialize(util::declval<T>()); })
        constexpr meta::WriterResult<void, Writer> serialize(container::StringView key, T&& value) {
            DI_TRY(m_serializer.get().serialize_key(key));
            auto guard = util::ScopeValueChange(m_serializer.get().m_state, State::Value);
            return m_serializer.get().serialize(value);
        }

    private:
        util::ReferenceWrapper<JsonSerializer> m_serializer;
    };

public:
    template<typename T>
    requires(concepts::ConstructibleFrom<Writer, T>)
    constexpr explicit JsonSerializer(T&& writer, JsonSerializerConfig config = {})
        : m_writer(util::forward<T>(writer)), m_config(config) {}

    constexpr meta::WriterResult<void, Writer> serialize_bool(bool value) {
        if (value) {
            return serialize_true();
        }
        return serialize_false();
    }

    constexpr meta::WriterResult<void, Writer> serialize_true() {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, "true"_sv));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_false() {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, "false"_sv));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_string(container::StringView view) {
        DI_TRY(serialize_comma());

        DI_TRY(io::write_exactly(m_writer, '"'));
        // FIXME: escape the string if needed.
        DI_TRY(io::write_exactly(m_writer, view));
        DI_TRY(io::write_exactly(m_writer, '"'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_number(concepts::Integral auto number) {
        DI_TRY(serialize_comma());

        using Enc = container::string::Utf8Encoding;
        using TargetContext = format::BoundedFormatContext<Enc, meta::SizeConstant<256>>;
        auto context = TargetContext {};
        DI_TRY(di::format::vpresent_encoded_context<Enc>(
            di::container::string::StringViewImpl<Enc>(encoding::assume_valid, u8"{}", 2),
            di::format::make_constexpr_format_args(number), context));

        DI_TRY(io::write_exactly(m_writer, context.output()));
        return {};
    }

    template<concepts::InvocableTo<meta::WriterResult<void, Writer>, JsonSerializer&> F>
    constexpr meta::WriterResult<void, Writer> serialize_array(F&& function) {
        DI_TRY(serialize_array_begin());
        auto guard = util::ScopeValueChange(m_state, State::First);
        DI_TRY(function::invoke(util::forward<F>(function), *this));
        return serialize_array_end();
    }

    template<concepts::InvocableTo<meta::WriterResult<void, Writer>, ObjectSerializerProxy&> F>
    constexpr meta::WriterResult<void, Writer> serialize_object(F&& function) {
        DI_TRY(serialize_object_begin());
        auto guard = util::ScopeValueChange(m_state, State::First);
        auto proxy = ObjectSerializerProxy(*this);
        DI_TRY(function::invoke(util::forward<F>(function), proxy));
        return serialize_object_end();
    }

    template<concepts::ReflectableToFields T>
    constexpr meta::WriterResult<void, Writer> serialize(T&& value) {
        return serialize_object([&](auto& serializer) -> meta::WriterResult<void, Writer> {
            return vocab::tuple_sequence<meta::WriterResult<void, Writer>>(
                [&](auto field) {
                    constexpr auto name = container::fixed_string_to_utf8_string_view<field.name>();
                    return serializer.serialize(name, field.get(value));
                },
                reflection::reflect(value));
        });
    }

    template<concepts::ReflectableToAtom T, auto atom = reflection::reflect(in_place_type<meta::RemoveCVRef<T>>)>
    requires(atom.is_bool() || atom.is_string() || atom.is_integer())
    constexpr meta::WriterResult<void, Writer> serialize(T&& value) {
        if constexpr (atom.is_bool()) {
            return serialize_bool(atom.get(value));
        } else if constexpr (atom.is_string()) {
            return serialize_string(atom.get(value));
        } else if constexpr (atom.is_integer()) {
            return serialize_number(atom.get(value));
        }
    }

    template<concepts::ReflectableToAtom T, auto atom = reflection::reflect(in_place_type<meta::RemoveCVRef<T>>)>
    requires(atom.is_list() &&
             requires { util::declval<JsonSerializer&>().serialize(util::declval<meta::ContainerReference<T>>()); })
    constexpr meta::WriterResult<void, Writer> serialize(T&& value) {
        return serialize_array([&](auto& serializer) -> meta::WriterResult<void, Writer> {
            return container::sequence(atom.get(value), [&](auto&& element) {
                return serializer.serialize(element);
            });
        });
    }

    template<concepts::ReflectableToAtom T, auto atom = reflection::reflect(in_place_type<meta::RemoveCVRef<T>>)>
    requires(atom.is_map() && concepts::detail::ConstantString<meta::TupleElement<meta::ContainerValue<T>, 0>> &&
             requires {
                 util::declval<JsonSerializer&>().serialize(
                     util::declval<meta::TupleValue<decltype(util::declval<meta::ContainerReference<T>>()), 1>>());
             })
    constexpr meta::WriterResult<void, Writer> serialize(T&& value) {
        return serialize_object([&](auto& serializer) -> meta::WriterResult<void, Writer> {
            return container::sequence(atom.get(value), [&](concepts::TupleLike auto&& element) {
                return serializer.serialize(util::get<0>(element), util::get<1>(element));
            });
        });
    }

    constexpr Writer const& writer() const& { return m_writer; }
    constexpr Writer&& writer() && { return util::move(m_writer); }

private:
    constexpr meta::WriterResult<void, Writer> serialize_comma() {
        if (m_state == State::Value) {
            return {};
        }
        if (util::exchange(m_state, State::Normal) == State::Normal) {
            DI_TRY(io::write_exactly(m_writer, ','));
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        } else if (m_indent > 0) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_object_begin() {
        DI_TRY(serialize_comma());
        DI_TRY(io::write_exactly(m_writer, '{'));
        ++m_indent;
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_object_end() {
        --m_indent;
        if (m_state != State::First) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        DI_TRY(io::write_exactly(m_writer, '}'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_array_begin() {
        DI_TRY(serialize_comma());
        DI_TRY(io::write_exactly(m_writer, '['));
        ++m_indent;
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_array_end() {
        --m_indent;
        if (m_state != State::First) {
            DI_TRY(serialize_newline());
            DI_TRY(serialize_indent());
        }
        DI_TRY(io::write_exactly(m_writer, ']'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_key(container::StringView key) {
        DI_TRY(serialize_string(key));
        DI_TRY(io::write_exactly(m_writer, ':'));
        if (pretty_print()) {
            DI_TRY(io::write_exactly(m_writer, ' '));
        }
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_newline() {
        if (!pretty_print()) {
            return {};
        }
        return io::write_exactly(m_writer, '\n');
    }

    constexpr meta::WriterResult<void, Writer> serialize_indent() {
        if (!pretty_print()) {
            return {};
        }
        return view::range(m_indent * m_config.indent_width()) | container::sequence([&](auto) {
                   return io::write_exactly(m_writer, ' ');
               });
    }

    constexpr bool pretty_print() const { return m_config.is_pretty(); }

    Writer m_writer;
    JsonSerializerConfig m_config {};
    usize m_indent { 0 };
    State m_state { State::First };
};

template<typename T>
JsonSerializer(T&&) -> JsonSerializer<T>;

template<typename T>
JsonSerializer(T&&, JsonSerializerConfig) -> JsonSerializer<T>;
}

namespace di {
using serialization::JsonSerializer;
using serialization::JsonSerializerConfig;
}
