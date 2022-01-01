#pragma once

#include <liim/format.h>
#include <liim/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Ext {
class Path {
public:
    static Maybe<Path> resolve(const String& path);
    static Path root();

    Path();
    ~Path();

    String basename() const;

    enum class SlashTerminated { No, Yes };
    String dirname(SlashTerminated slash_terminated = SlashTerminated::No) const;

    String to_string() const;

    void set_to_parent();

    Path join_component(const String& name) const;
    const Vector<String>& components() const { return m_components; }

private:
    Path(const String& path);

    Vector<String> m_components;
};
}

namespace LIIM::Format {
template<>
struct Formatter<Ext::Path> : public Formatter<String> {
    void format(const Ext::Path& path, FormatContext& context) {
        return Formatter<String>::format(::format("{}", path.to_string()), context);
    }
};
}
