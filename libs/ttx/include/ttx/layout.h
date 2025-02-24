#pragma once

#include "di/reflect/prelude.h"
#include "di/vocab/pointer/box.h"
#include "dius/tty.h"
#include "ttx/pane.h"

namespace ttx {
// Represents the direction of splits.
enum class Direction {
    None, // Case when is 0 or 1 children.
    Horizontal,
    Vertical,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Direction>) {
    using enum Direction;
    return di::make_enumerators<"Direction">(di::enumerator<"None", None>, di::enumerator<"Horitzontal", Horizontal>,
                                             di::enumerator<"Vertical", Vertical>);
}

struct LayoutNode;
class LayoutGroup;

// Represents the layout result for a single pane. The row and col
// coordinates are absolute.
struct LayoutEntry {
    u32 row { 0 };
    u32 col { 0 };
    dius::tty::WindowSize size;
    LayoutNode* parent { nullptr };
    Pane* pane { nullptr };

    auto operator==(LayoutEntry const&) const -> bool = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<LayoutEntry>) {
        return di::make_fields<"LayoutEntry">(di::field<"row", &LayoutEntry::row>, di::field<"col", &LayoutEntry::col>,
                                              di::field<"size", &LayoutEntry::size>,
                                              di::field<"parent", &LayoutEntry::parent>,
                                              di::field<"pane", &LayoutEntry::pane>);
    }
};

// Represents a full layout tree. This is created by calling
// LayoutGroup::layout().
struct LayoutNode {
    u32 row { 0 };
    u32 col { 0 };
    dius::tty::WindowSize size;
    di::Vector<di::Variant<di::Box<LayoutNode>, LayoutEntry>> children;
    LayoutGroup* group { nullptr };
    Direction direction { Direction::None };

    auto find_pane(Pane* pane) -> di::Optional<LayoutEntry&>;
    auto hit_test(u32 row, u32 col) -> di::Optional<LayoutEntry&>;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<LayoutNode>) {
        return di::make_fields<"LayoutNode">(
            di::field<"row", &LayoutNode::row>, di::field<"col", &LayoutNode::col>,
            di::field<"size", &LayoutNode::size>, di::field<"children", &LayoutNode::children>,
            di::field<"group", &LayoutNode::group>, di::field<"direction", &LayoutNode::direction>);
    }
};

// Represents a group of panes in a hierarchy. Instead of using a strict binary tree,
// we allow multiple children on a single level so that by default, splits made in
// the same direction share space evenly.
class LayoutGroup {
public:
    constexpr auto direction() const -> Direction { return m_direction; }
    constexpr auto empty() const -> bool { return m_children.empty(); }
    constexpr auto single() const -> bool { return m_children.size() == 1; }

    // NOTE: this method returns the correct size for the new pane, and a lvalue reference where
    // the caller should store its newly created Pane. We need this akward API so that we can
    // create a Pane with a sane initial size.
    auto split(dius::tty::WindowSize const& size, u32 row_offset, u32 col_offset, Pane* reference, Direction direction)
        -> di::Tuple<di::Box<LayoutNode>, di::Optional<LayoutEntry&>, di::Optional<di::Box<Pane>&>>;

    // NOTE: after removing a pane, calling layout() is necessary as any previous LayoutNode's may become invalid.
    void remove_pane(Pane* pane);

    // NOTE: in addition to computing the layout tree, Pane::resize() is called to inform each Pane of its (potentially)
    // new size.
    auto layout(dius::tty::WindowSize const& size, u32 row_offset, u32 col_offset) -> di::Box<LayoutNode>;

private:
    friend struct FindPaneInLayoutGroup;

    di::Vector<di::Variant<di::Box<LayoutGroup>, di::Box<Pane>>> m_children;
    Direction m_direction { Direction::None };
};
}
