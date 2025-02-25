#include "ttx/layout.h"

#include "di/container/interface/erase.h"
#include "di/container/view/as_rvalue.h"
#include "di/function/overload.h"
#include "di/util/scope_exit.h"
#include "di/vocab/pointer/box.h"
#include "di/vocab/variant/get_if.h"
#include "di/vocab/variant/holds_alternative.h"
#include "dius/tty.h"

namespace ttx {
struct FindPaneInLayoutGroup {
    static auto operator()(LayoutGroup* parent, usize index, Pane* target, Pane* pane)
        -> di::Tuple<LayoutGroup*, usize> {
        if (pane == target) {
            return { parent, index };
        }
        return {};
    }

    static auto operator()(LayoutGroup*, usize, Pane* target, LayoutGroup* group) -> di::Tuple<LayoutGroup*, usize> {
        for (auto [i, child] : di::enumerate(group->m_children)) {
            auto [parent, index] = di::visit(
                [&](auto& c) {
                    return FindPaneInLayoutGroup {}(group, i, target, c.get());
                },
                child);
            if (parent) {
                return { parent, index };
            }
        }
        return {};
    }
};

auto LayoutNode::find_pane(Pane* pane) -> di::Optional<LayoutEntry&> {
    for (auto& child : children) {
        auto result = di::visit(di::overload(
                                    [&](LayoutEntry& entry) -> di::Optional<LayoutEntry&> {
                                        if (entry.pane == pane) {
                                            return entry;
                                        }
                                        return {};
                                    },
                                    [&](di::Box<LayoutNode>& node) -> di::Optional<LayoutEntry&> {
                                        return node->find_pane(pane);
                                    }),
                                child);
        if (result) {
            return result;
        }
    }
    return {};
}

auto LayoutNode::hit_test(u32 row, u32 col) -> di::Optional<LayoutEntry&> {
    if (row >= this->row + size.rows || col >= this->col + size.cols) {
        return {};
    }
    for (auto& child : children) {
        auto result = di::visit(di::overload(
                                    [&](LayoutEntry& entry) -> di::Optional<LayoutEntry&> {
                                        if (row >= entry.row && row < entry.row + entry.size.rows && col >= entry.col &&
                                            col < entry.col + entry.size.cols) {
                                            return entry;
                                        }
                                        return {};
                                    },
                                    [&](di::Box<LayoutNode>& node) -> di::Optional<LayoutEntry&> {
                                        return node->hit_test(row, col);
                                    }),
                                child);
        if (result) {
            return result;
        }
    }
    return {};
}

auto LayoutNode::hit_test_vertical_line(u32 col, u32 row_start, u32 row_end) -> di::TreeSet<LayoutEntry*> {
    auto result = di::TreeSet<LayoutEntry*> {};
    for (auto& child : children) {
        auto res = di::visit(di::overload(
                                 [&](LayoutEntry& entry) -> di::TreeSet<LayoutEntry*> {
                                     auto line_intersects =
                                         !(row_end < entry.row || row_start >= entry.row + entry.size.rows);
                                     if (line_intersects && col >= entry.col && col < entry.col + entry.size.cols) {
                                         return di::single(&entry) | di::to<di::TreeSet>();
                                     }
                                     return {};
                                 },
                                 [&](di::Box<LayoutNode>& node) -> di::TreeSet<LayoutEntry*> {
                                     return node->hit_test_vertical_line(col, row_start, row_end);
                                 }),
                             child);
        result.insert_container(di::move(res));
    }
    return result;
}

auto LayoutNode::hit_test_horizontal_line(u32 row, u32 col_start, u32 col_end) -> di::TreeSet<LayoutEntry*> {
    auto result = di::TreeSet<LayoutEntry*> {};
    for (auto& child : children) {
        auto res = di::visit(di::overload(
                                 [&](LayoutEntry& entry) -> di::TreeSet<LayoutEntry*> {
                                     auto line_intersects =
                                         !(col_end < entry.col || col_start >= entry.col + entry.size.cols);
                                     if (line_intersects && row >= entry.row && row < entry.row + entry.size.rows) {
                                         return di::single(&entry) | di::to<di::TreeSet>();
                                     }
                                     return {};
                                 },
                                 [&](di::Box<LayoutNode>& node) -> di::TreeSet<LayoutEntry*> {
                                     return node->hit_test_horizontal_line(row, col_start, col_end);
                                 }),
                             child);
        result.insert_container(di::move(res));
    }
    return result;
}

auto LayoutGroup::split(dius::tty::WindowSize const& size, u32 row_offset, u32 col_offset, Pane* reference,
                        Direction direction)
    -> di::Tuple<di::Box<LayoutNode>, di::Optional<LayoutEntry&>, di::Optional<di::Box<Pane>&>> {
    auto do_final_layout = [&](di::Box<Pane>& child)
        -> di::Tuple<di::Box<LayoutNode>, di::Optional<LayoutEntry&>, di::Optional<di::Box<Pane>&>> {
        auto result = layout(size, row_offset, col_offset);
        return { di::move(result), result->find_pane(child.get()), child };
    };

    // Init case: we're adding the first pane.
    if (empty()) {
        ASSERT(reference == nullptr);

        m_direction = Direction::None;
        auto& child = m_children.emplace_back(di::Box<Pane>());
        return do_final_layout(child.get<di::Box<Pane>>());
    }

    // Recursive case: find the reference node in the tree.
    ASSERT(direction != Direction::None);
    auto [parent, index] = FindPaneInLayoutGroup {}(nullptr, 0, reference, this);
    if (!parent) {
        return {};
    }
    ASSERT(index < parent->m_children.size());
    ASSERT(di::holds_alternative<di::Box<Pane>>(parent->m_children[index]));

    // Base case: we only have 1 pane so we're yet to establish a direction.
    if (parent->single()) {
        ASSERT(di::holds_alternative<di::Box<Pane>>(m_children[0]));

        parent->m_direction = direction;
        auto& child = parent->m_children.emplace_back(di::Box<Pane>());
        return do_final_layout(child.get<di::Box<Pane>>());
    }

    // Case 2: the parent's direction is the same as the requested direction.
    if (parent->m_direction == direction) {
        auto child = parent->m_children.emplace(parent->m_children.begin() + index + 1, di::Box<Pane>());
        return do_final_layout(child->get<di::Box<Pane>>());
    }

    // Case 3: the parent' direction differs. We need to create a new LayoutGroup.
    auto new_group = di::make_box<LayoutGroup>();
    new_group->m_direction = direction;
    new_group->m_children.push_back(di::move(parent->m_children[index].get<di::Box<Pane>>()));
    auto& child = new_group->m_children.emplace_back(di::Box<Pane>());
    parent->m_children[index] = di::move(new_group);
    return do_final_layout(child.get<di::Box<Pane>>());
}

void LayoutGroup::remove_pane(Pane* pane) {
    // First, try to delete the pane from this level.
    di::erase_if(m_children, [&](auto const& variant) {
        return di::visit(di::overload(
                             [&](di::Box<LayoutGroup> const&) {
                                 return false;
                             },
                             [&](di::Box<Pane> const& box) {
                                 return box.get() == pane;
                             }),
                         variant);
    });

    // Then, try to delete the pane recursively.
    for (auto const& child : m_children) {
        if (auto group = di::get_if<di::Box<LayoutGroup>>(child)) {
            group.value()->remove_pane(pane);
        }
    }

    // Now determine if we have any immediate children which are either empty or singular.
    // If so, we need to either remove or shrink our children.
    for (auto it = m_children.begin(); it != m_children.end();) {
        auto advance = di::ScopeExit([&] {
            ++it;
        });

        auto group = di::get_if<di::Box<LayoutGroup>>(*it);
        if (!group) {
            continue;
        }

        // If empty, remove the node entirely.
        if (group.value()->empty()) {
            advance.release();
            it = m_children.erase(it);
            continue;
        }

        // If our child is a layout group with the same direction as us, we need to
        // absorb all its children.
        if (group.value()->direction() == this->direction()) {
            // First erase the child and then insert all its children.
            auto save = di::get<di::Box<LayoutGroup>>(di::move(*it));
            advance.release();
            it = m_children.erase(it);

            it = m_children.insert_container(it, di::move(save->m_children) | di::as_rvalue).end();
            continue;
        }
    }

    // If we're single, and our sole child is a layout group, we need to replace ourselves with our child.
    if (single() && di::holds_alternative<di::Box<LayoutGroup>>(m_children[0])) {
        auto save = di::move(m_children[0]);
        *this = di::move(*di::get<di::Box<LayoutGroup>>(save));
    }

    // Clear our direction if don't have multiple children.
    if (m_children.size() <= 1) {
        m_direction = Direction::None;
    }
}

static void resize_pane(Pane* pane, dius::tty::WindowSize const& size) {
    if (pane) {
        pane->resize(size);
    }
};

auto LayoutGroup::layout(dius::tty::WindowSize const& size, u32 row_offset, u32 col_offset) -> di::Box<LayoutNode> {
    auto node = di::make_box<LayoutNode>(
        row_offset, col_offset, size, di::Vector<di::Variant<di::Box<LayoutNode>, LayoutEntry>> {}, this, direction());

    // Base case: we less than 1 pane, so allocate all of our space to it.
    if (m_direction == Direction::None) {
        ASSERT(empty() || single());
        if (single()) {
            auto pane = di::get_if<di::Box<Pane>>(m_children[0]);
            ASSERT(pane.has_value());

            resize_pane(pane.value().get(), size);
            node->children.push_back(LayoutEntry { row_offset, col_offset, size, node.get(), pane.value().get() });
        }
        return node;
    }

    auto const fixed_dimension =
        m_direction == Direction::Horizontal ? &dius::tty::WindowSize::rows : &dius::tty::WindowSize::cols;
    auto const fixed_pixels_dimension = m_direction == Direction::Horizontal ? &dius::tty::WindowSize::pixel_height
                                                                             : &dius::tty::WindowSize::pixel_width;
    auto const dynamic_dimension =
        m_direction == Direction::Horizontal ? &dius::tty::WindowSize::cols : &dius::tty::WindowSize::rows;
    auto const dynamic_pixels_dimension = m_direction == Direction::Horizontal ? &dius::tty::WindowSize::pixel_width
                                                                               : &dius::tty::WindowSize::pixel_height;

    // We need at least 1 cell for each child, and N - 1 cells for the box drawing separator between panes.
    auto const min_dynamic_size = m_children.size() * 2 - 1;
    auto const min_fixed_size = 1_u32;
    if (size.*fixed_dimension < min_fixed_size || size.*dynamic_dimension < min_dynamic_size) {
        return node;
    }

    // Account for the N - 1 cells of padding between panes.
    auto available_size = size;
    available_size.*dynamic_pixels_dimension -=
        (m_children.size() - 1) * available_size.*dynamic_pixels_dimension / available_size.*dynamic_dimension;
    available_size.*dynamic_dimension -= m_children.size() - 1;

    auto const target_dynamic_size = di::Rational(i32(available_size.*dynamic_dimension), i32(m_children.size()));
    auto leftover_dynamic_size = di::Rational(i32(0));

    auto const fixed_size = available_size.*fixed_dimension;
    auto const fixed_pixels = available_size.*fixed_pixels_dimension;

    auto fixed_offset = m_direction == Direction::Horizontal ? row_offset : col_offset;
    auto dynamic_offset = m_direction == Direction::Horizontal ? col_offset : row_offset;

    for (auto const& child : m_children) {
        leftover_dynamic_size += target_dynamic_size;
        auto dynamic_size = leftover_dynamic_size.round();
        auto dynamic_pixels =
            dynamic_size * available_size.*dynamic_pixels_dimension / available_size.*dynamic_dimension;

        auto size = dius::tty::WindowSize {
            m_direction == Direction::Horizontal ? fixed_size : dynamic_size,
            m_direction == Direction::Horizontal ? dynamic_size : fixed_size,
            m_direction == Direction::Horizontal ? dynamic_pixels : fixed_pixels,
            m_direction == Direction::Horizontal ? fixed_pixels : dynamic_pixels,
        };
        auto row = m_direction == Direction::Horizontal ? fixed_offset : dynamic_offset;
        auto col = m_direction == Direction::Horizontal ? dynamic_offset : fixed_offset;

        // Do recursive layout.
        di::visit(di::overload(
                      [&](di::Box<Pane> const& pane) {
                          resize_pane(pane.get(), size);
                          node->children.emplace_back(LayoutEntry {
                              row,
                              col,
                              size,
                              node.get(),
                              pane.get(),
                          });
                      },
                      [&](di::Box<LayoutGroup> const& group) {
                          node->children.emplace_back(group->layout(size, row, col));
                      }),
                  child);

        leftover_dynamic_size -= dynamic_size;
        dynamic_offset += dynamic_size;
        dynamic_offset++; // Account for box drawing character.
    }

    return node;
}
}
