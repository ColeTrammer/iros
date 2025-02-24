#include "dius/test/prelude.h"
#include "dius/tty.h"
#include "ttx/layout.h"

namespace layout {
using namespace ttx;

static auto add_pane(LayoutGroup& root, dius::tty::WindowSize const& size, Pane* reference, Direction direction)
    -> di::Tuple<Pane&, di::Box<LayoutNode>> {
    auto [layout_tree, entry, pane_out] = root.split(size, 0, 0, reference, direction);
    ASSERT(entry);
    ASSERT(pane_out);
    auto& pane = *pane_out = Pane::create_mock();
    entry->pane = pane.get();

    return { *pane, di::move(layout_tree) };
};

static auto validate_layout_for_pane(Pane& pane, LayoutNode& tree, u32 row, u32 col, dius::tty::WindowSize size) {
    auto entry = tree.find_pane(&pane);
    ASSERT(entry);

    // Default the pixel widths/heights.
    size.pixel_height = size.rows * 10;
    size.pixel_width = size.cols * 10;

    ASSERT_EQ(entry->pane, &pane);
    ASSERT_EQ(entry->row, row);
    ASSERT_EQ(entry->col, col);
    ASSERT_EQ(entry->size, size);
};

static void splits() {
    constexpr auto size = dius::tty::WindowSize(64, 128, 1280, 640);

    auto root = LayoutGroup {};

    // Initial pane
    auto [pane0, l0] = add_pane(root, size, nullptr, Direction::None);
    validate_layout_for_pane(pane0, *l0, 0, 0, size);

    // Vertical split
    auto [pane1, l1] = add_pane(root, size, &pane0, Direction::Vertical);
    validate_layout_for_pane(pane0, *l1, 0, 0, { 32, 128 });
    validate_layout_for_pane(pane1, *l1, 33, 0, { 31, 128 });

    // Horitzontal split above
    auto [pane2, l2] = add_pane(root, size, &pane0, Direction::Horizontal);
    validate_layout_for_pane(pane0, *l2, 0, 0, { 32, 64 });
    validate_layout_for_pane(pane1, *l2, 33, 0, { 31, 128 });
    validate_layout_for_pane(pane2, *l2, 0, 65, { 32, 63 });

    // 2 Veritcal splits under pane 2.
    auto [pane4, _] = add_pane(root, size, &pane2, Direction::Vertical);
    auto [pane3, l3] = add_pane(root, size, &pane2, Direction::Vertical);
    validate_layout_for_pane(pane0, *l3, 0, 0, { 32, 64 });
    validate_layout_for_pane(pane1, *l3, 33, 0, { 31, 128 });
    validate_layout_for_pane(pane2, *l3, 0, 65, { 10, 63 });
    validate_layout_for_pane(pane3, *l3, 11, 65, { 10, 63 });
    validate_layout_for_pane(pane4, *l3, 22, 65, { 10, 63 });
}

static void remove_pane() {
    constexpr auto size = dius::tty::WindowSize(64, 128, 1280, 640);

    auto root = LayoutGroup {};

    // Initial pane
    auto [pane0, _] = add_pane(root, size, nullptr, Direction::None);

    // Vertical split
    auto [pane1, _] = add_pane(root, size, &pane0, Direction::Vertical);

    // Horitzontal split above
    auto [pane2, _] = add_pane(root, size, &pane0, Direction::Horizontal);

    // 2 Veritcal splits under pane 2.
    auto [pane4, _] = add_pane(root, size, &pane2, Direction::Vertical);
    auto [pane3, _] = add_pane(root, size, &pane2, Direction::Vertical);

    // Now the layout looks something like this:
    // |---------|--------|
    // |0        |2       |
    // |         |--------|
    // |         |3       |
    // |         |--------|
    // |         |4       |
    // |---------|--------|
    // |1                 |
    // |                  |
    // |                  |
    // |                  |
    // |                  |
    // |------------------|

    // When we remove pane 0, we need to collapse panes 2-4, into the same vertical layout
    // group with pane 1.
    root.remove_pane(&pane0);

    auto l0 = root.layout(size, 0, 0);
    validate_layout_for_pane(pane2, *l0, 0, 0, { 15, 128 });
    validate_layout_for_pane(pane3, *l0, 16, 0, { 16, 128 });
    validate_layout_for_pane(pane4, *l0, 33, 0, { 15, 128 });
    validate_layout_for_pane(pane1, *l0, 49, 0, { 15, 128 });
}

TEST(layout, splits)
TEST(layout, remove_pane)
}
