#pragma once

#include <di/container/algorithm/all_of.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/path/path_iterator.h>
#include <di/container/string/string_view.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
template<concepts::Encoding Enc>
class PathViewImpl;

template<typename Self, concepts::Encoding Enc>
class ConstantPathInterface {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using ViewIter = meta::ContainerIterator<View>;
    using PathView = PathViewImpl<Enc>;

    constexpr auto self() -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const -> Self const& { return static_cast<Self const&>(*this); }

    constexpr auto data() const -> View { return self().data(); }

public:
    constexpr auto empty() const -> bool { return data().empty(); }

    constexpr auto view() const& -> PathView { return PathView(data()); }
    constexpr operator PathView() const& { return view(); }

    constexpr auto front() const {
        return lift_bool(!empty()) % [&] {
            return *begin();
        };
    }
    constexpr auto back() const {
        return lift_bool(!empty()) % [&] {
            return *--end();
        };
    }

    constexpr auto begin() const { return PathIterator(data(), { data().begin(), m_first_component_end }); }
    constexpr auto end() const { return PathIterator(data(), { data().end(), data().end() }); }

    constexpr auto is_absolute() const -> bool { return data().starts_with(CodePoint('/')); }
    constexpr auto is_relative() const -> bool { return !is_absolute(); }

    constexpr auto filename() const -> Optional<View> {
        return lift_bool(!empty() && !data().ends_with(CodePoint('/'))) % [&] {
            auto trailing_slash = data().rfind(CodePoint('/'));
            if (!trailing_slash) {
                return data();
            }
            return data().substr(trailing_slash.end());
        };
    }

    constexpr auto extension() const -> Optional<View> {
        auto filename = this->filename();
        if (!filename) {
            return nullopt;
        }
        auto split = split_filename(*filename);
        return util::get<1>(split);
    }

    constexpr auto stem() const -> Optional<View> {
        auto filename = this->filename();
        if (!filename) {
            return nullopt;
        }
        auto split = split_filename(*filename);
        return util::get<0>(split);
    }

    constexpr auto parent_path() const -> Optional<PathView> {
        return lift_bool(!empty() && !container::all_of(data(), function::equal(CodePoint('/')))) >> [&] {
            auto result = PathView(strip_filename(data()));
            return lift_bool(!result.empty()) % [&] {
                return result;
            };
        };
    }

    constexpr auto starts_with(PathView prefix) const -> bool { return container::starts_with(*this, prefix); }
    constexpr auto ends_with(PathView suffix) const -> bool { return container::ends_with(*this, suffix); }

    constexpr auto filename_ends_with(View suffix) const -> bool {
        return filename() % [&](auto filename) {
            return filename.ends_with(suffix);
        } == true;
    }

    constexpr auto strip_prefix(PathView prefix) -> Optional<PathView> {
        auto [a, b] = container::mismatch(*this, prefix);
        if (b != prefix.end()) {
            return nullopt;
        }
        return PathView(View(a.current_data(), this->end().current_data()));
    }

protected:
    constexpr void compute_first_component_end() {
        if (data().starts_with(CodePoint('/'))) {
            m_first_component_end = container::next(data().begin());
        } else {
            m_first_component_end = container::find(data(), CodePoint('/'));
        }
    }

private:
    constexpr friend auto operator==(Self const& a, Self const& b) -> bool { return container::equal(a, b); }
    constexpr friend auto operator<=>(Self const& a, Self const& b) { return container::compare(a, b); }

    constexpr static auto split_filename(View filename) -> Tuple<Optional<View>, Optional<View>> {
        auto last_dot_view = filename.rfind(CodePoint('.'));
        if (!last_dot_view || last_dot_view.begin() == filename.begin()) {
            return { filename, nullopt };
        }
        return { filename.substr(filename.begin(), last_dot_view.begin()), filename.substr(last_dot_view.end()) };
    }

    constexpr static auto strip_filename(View view) -> View {
        while (view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        while (!view.empty() && !view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        while (view.size() > 2 && view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        return view;
    }

    ViewIter m_first_component_end {};
};
}
