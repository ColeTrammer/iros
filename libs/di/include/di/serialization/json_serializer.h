#pragma once

#include <di/container/string/string_impl.h>
#include <di/container/string/string_view.h>
#include <di/container/string/utf8_encoding.h>
#include <di/container/vector/static_vector.h>
#include <di/function/invoke.h>
#include <di/io/prelude.h>
#include <di/io/write_exactly.h>
#include <di/util/exchange.h>
#include <di/util/reference_wrapper.h>
#include <di/util/scope_value_change.h>
#include <di/vocab/error/result.h>

namespace di::serialization {
template<concepts::Impl<Writer> Writer>
class JsonSerializer {
private:
    class ObjectSerializerProxy {
    public:
        constexpr explicit ObjectSerializerProxy(JsonSerializer& serializer) : m_serializer(serializer) {}

        template<concepts::InvocableTo<meta::WriterResult<void, Writer>, JsonSerializer&> F>
        constexpr meta::WriterResult<void, Writer> serialize_key(container::StringView name, F&& function) {
            DI_TRY(m_serializer.get().serialize_string(name));
            DI_TRY(io::write_exactly(m_serializer.get().m_writer, ':'));

            auto guard = util::ScopeValueChange(m_serializer.get().m_first, true);
            DI_TRY(function::invoke(util::forward<F>(function), m_serializer.get()));
            return {};
        }

    private:
        util::ReferenceWrapper<JsonSerializer> m_serializer;
    };

public:
    constexpr explicit JsonSerializer(Writer writer) : m_writer(util::move(writer)) {}

    constexpr meta::WriterResult<void, Writer> serialize_string(container::StringView view) {
        if (!util::exchange(m_first, false)) {
            DI_TRY(io::write_exactly(m_writer, ','));
        }

        DI_TRY(io::write_exactly(m_writer, '"'));
        // FIXME: escape the string if needed.
        DI_TRY(io::write_exactly(m_writer, view));
        DI_TRY(io::write_exactly(m_writer, '"'));
        return {};
    }

    constexpr meta::WriterResult<void, Writer> serialize_number(concepts::Integral auto number) {
        if (!util::exchange(m_first, false)) {
            DI_TRY(io::write_exactly(m_writer, ','));
        }

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
        if (!util::exchange(m_first, false)) {
            DI_TRY(io::write_exactly(m_writer, ','));
        }

        DI_TRY(io::write_exactly(m_writer, '['));
        auto guard = util::ScopeValueChange(m_first, true);
        DI_TRY(function::invoke(util::forward<F>(function), *this));
        DI_TRY(io::write_exactly(m_writer, ']'));
        return {};
    }

    template<concepts::InvocableTo<meta::WriterResult<void, Writer>, ObjectSerializerProxy&> F>
    constexpr meta::WriterResult<void, Writer> serialize_object(F&& function) {
        if (!util::exchange(m_first, false)) {
            DI_TRY(io::write_exactly(m_writer, ','));
        }

        DI_TRY(io::write_exactly(m_writer, '{'));
        auto guard = util::ScopeValueChange(m_first, true);
        auto proxy = ObjectSerializerProxy(*this);
        DI_TRY(function::invoke(util::forward<F>(function), proxy));
        DI_TRY(io::write_exactly(m_writer, '}'));
        return {};
    }

    constexpr Writer const& writer() const& { return m_writer; }
    constexpr Writer writer() && { return util::move(m_writer); }

private:
    Writer m_writer;
    bool m_first { true };
};
}

namespace di {
using serialization::JsonSerializer;
}
