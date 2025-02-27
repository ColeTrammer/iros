#include "di/cli/parser.h"
#include "di/container/queue/queue.h"
#include "di/container/ring/ring.h"
#include "di/container/string/string_view.h"
#include "di/io/writer_print.h"
#include "di/sync/synchronized.h"
#include "di/util/construct.h"
#include "dius/main.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"
#include "dius/tty.h"
#include "ttx/focus_event.h"
#include "ttx/layout.h"
#include "ttx/pane.h"
#include "ttx/terminal_input.h"
#include "ttx/utf8_stream_decoder.h"

namespace ttx {
struct Args {
    di::Vector<di::TransparentStringView> command;
    bool help { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("ttx"_sv, "Terminal multiplexer"_sv)
            .argument<&Args::command>("COMMAND"_sv, "Program to run in terminal"_sv, true)
            .help();
    }
};

struct Tab;

struct PaneExited {
    Tab* tab = nullptr;
    Pane* pane = nullptr;
};

using RenderEvent = di::Variant<dius::tty::WindowSize, PaneExited>;

// Corresponds to tmux window.
struct Tab {
    di::String name;
    LayoutGroup layout_root {};
    di::Box<LayoutNode> layout_tree {};
    di::Ring<Pane*> panes_ordered_by_recency {};
    Pane* active { nullptr };
};

struct LayoutState {
    dius::tty::WindowSize size;
    di::Vector<di::Box<Tab>> tabs {};
    Tab* active_tab { nullptr };
    di::Queue<RenderEvent> events {};
};

static auto main(Args& args) -> di::Result<void> {
    [[maybe_unused]] auto& log = dius::stderr = TRY(dius::open_sync("/tmp/ttx.log"_pv, dius::OpenMode::WriteClobber));

    auto done = di::Atomic<bool>(false);

    auto set_done = [&] {
        // TODO: timeout/skip waiting for processes to die after sending SIGHUP.
        if (!done.exchange(true, di::MemoryOrder::Release)) {
            // Ensure the SIGWINCH thread exits.
            (void) dius::system::ProcessHandle::self().signal(dius::Signal::WindowChange);
            // Ensure the input thread exits. (By requesting device attributes, thus waking up the input thread).
            (void) dius::stdin.write_exactly(di::as_bytes("\033[c"_sv.span()));
        }
    };

    auto layout_state = di::Synchronized(LayoutState {
        .size = TRY(dius::stdin.get_tty_window_size()),
    });

    auto invalidate_tab = [&](Tab& tab) {
        for (auto* pane : tab.panes_ordered_by_recency) {
            pane->invalidate_all();
        }
    };

    auto do_layout = [&](LayoutState& state, dius::tty::WindowSize size) {
        state.size = size;

        if (!state.active_tab) {
            return;
        }
        state.active_tab->layout_tree = state.active_tab->layout_root.layout(state.size.rows_shrinked(1), 1, 0);

        invalidate_tab(*state.active_tab);
    };

    auto set_active = [&](LayoutState& state, Tab& tab, Pane* pane) {
        if (tab.active == pane) {
            return;
        }

        // Unfocus the old pane, and focus the new pane.
        if (state.active_tab == &tab && tab.active) {
            tab.active->event(FocusEvent::focus_out());
        }
        tab.active = pane;
        if (pane) {
            di::erase(tab.panes_ordered_by_recency, pane);
            tab.panes_ordered_by_recency.push_front(pane);
        }
        if (state.active_tab == &tab && tab.active) {
            tab.active->event(FocusEvent::focus_in());
        }
    };

    auto set_active_tab = [&](LayoutState& state, Tab* tab) {
        if (state.active_tab == tab) {
            return;
        }

        // Unfocus the old pane, and focus the new pane.
        if (state.active_tab) {
            if (auto* pane = state.active_tab->active) {
                pane->event(FocusEvent::focus_out());
            }
        }
        state.active_tab = tab;
        if (state.active_tab) {
            if (auto* pane = state.active_tab->active) {
                pane->event(FocusEvent::focus_in());
            }
            invalidate_tab(*state.active_tab);
        }
    };

    auto remove_tab = [&](LayoutState& state, Tab& tab) {
        // Clear active tab.
        if (state.active_tab == &tab) {
            auto it = di::find(state.tabs, &tab, &di::Box<Tab>::get);
            if (it == state.tabs.end()) {
                set_active_tab(state, state.tabs.at(0).transform(&di::Box<Tab>::get).value_or(nullptr));
            } else if (state.tabs.size() == 1) {
                set_active_tab(state, nullptr);
            } else {
                auto index = usize(it - state.tabs.begin());
                if (index == state.tabs.size() - 1) {
                    set_active_tab(state, state.tabs[index - 1].get());
                } else {
                    set_active_tab(state, state.tabs[index + 1].get());
                }
            }
        }

        // Delete tab.
        di::erase_if(state.tabs, [&](di::Box<Tab> const& pointer) {
            return pointer.get() == &tab;
        });

        // Exit when no tabs are left.
        if (state.tabs.empty()) {
            set_done();
        }
    };

    auto remove_pane = [&](LayoutState& state, Tab& tab, Pane* pane) {
        // Clear active pane.
        if (tab.active == pane) {
            if (pane) {
                di::erase(tab.panes_ordered_by_recency, pane);
            }
            auto candidates = tab.panes_ordered_by_recency | di::transform([](Pane* pane) {
                                  return pane;
                              });
            set_active(state, tab, candidates.front().value_or(nullptr));
        }

        tab.layout_root.remove_pane(pane);
        do_layout(state, state.size);

        // Exit tab when there are no panes left.
        if (tab.layout_root.empty()) {
            remove_tab(state, tab);
        }
    };

    auto add_pane = [&](LayoutState& state, Tab& tab, di::Vector<di::TransparentStringView> command,
                        Direction direction) -> di::Result<> {
        auto [new_layout, pane_layout, pane_out] =
            tab.layout_root.split(state.size.rows_shrinked(1), 1, 0, tab.active, direction);

        if (!pane_layout || !pane_out || pane_layout->size == dius::tty::WindowSize {}) {
            // NOTE: this happens when the visible terminal size is too small.
            tab.layout_root.remove_pane(nullptr);
            return di::Unexpected(di::BasicError::InvalidArgument);
        }

        auto maybe_pane = Pane::create(di::move(command), pane_layout->size);
        if (!maybe_pane) {
            tab.layout_root.remove_pane(nullptr);
            return di::Unexpected(di::move(maybe_pane).error());
        }

        auto& pane = *pane_out = di::move(maybe_pane).value();
        pane_layout->pane = pane.get();
        tab.layout_tree = di::move(new_layout);

        pane->did_exit = di::make_function<void()>([&layout_state, &tab, pane = pane.get()] {
            layout_state.with_lock([&](LayoutState& state) {
                state.events.push(PaneExited(&tab, pane));
            });
        });

        set_active(state, tab, pane.get());
        return {};
    };

    auto add_tab = [&](LayoutState& state, di::Vector<di::TransparentStringView> command) -> di::Result<> {
        auto tab = di::make_box<Tab>();
        tab->name = command[0].to_owned() | di::transform([](char c) {
                        return c32(c);
                    }) |
                    di::to<di::String>();
        TRY(add_pane(state, *tab, di::move(command), Direction::None));

        set_active_tab(state, tab.get());
        state.tabs.push_back(di::move(tab));

        return {};
    };

    // Initial tab.
    TRY(add_tab(layout_state.get_assuming_no_concurrent_accesses(), di::clone(args.command)));

    // Setup - raw mode
    auto _ = TRY(dius::stdin.enter_raw_mode());

    // Setup - alternate screen buffer.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049h\033[H\033[2J"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049l\033[?25h"_sv);
    });

    // Setup - disable autowrap.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?7l"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?7h"_sv);
    });

    // Setup - kitty key mode.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[>31u"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[<u"_sv);
    });

    // Setup - capture all mouse events and use SGR mosue reporting.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1003h\033[?1006h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1006l\033[?1003l"_sv);
    });

    // Setup - enable focus events.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1004h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1004l"_sv);
    });

    // Setup - bracketed paste.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2004h"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?2004l"_sv);
    });

    TRY(dius::system::mask_signal(dius::Signal::WindowChange));

    auto input_thread = TRY(dius::Thread::create([&] -> void {
        auto _ = di::ScopeExit([&] {
            set_done();
        });

        constexpr auto prefix_key = Key::B;
        auto got_prefix = false;

        auto buffer = di::Vector<byte> {};
        buffer.resize(4096);

        auto parser = TerminalInputParser {};
        auto utf8_decoder = Utf8StreamDecoder {};
        while (!done.load(di::MemoryOrder::Acquire)) {
            auto nread = dius::stdin.read_some(buffer.span());
            if (!nread.has_value() || done.load(di::MemoryOrder::Acquire)) {
                break;
            }

            auto utf8_string = utf8_decoder.decode(buffer | di::take(*nread));
            auto events = parser.parse(utf8_string);
            for (auto const& event : events) {
                if (auto ev = di::get_if<KeyEvent>(event)) {
                    if (ev->type() == KeyEventType::Press &&
                        !(ev->key() > Key::ModifiersBegin && ev->key() < Key::ModifiersEnd)) {
                        auto reset_got_prefix = di::ScopeExit([&] {
                            got_prefix = false;
                        });

                        if (!got_prefix && ev->key() == prefix_key && !!(ev->modifiers() & Modifiers::Control)) {
                            got_prefix = true;
                            reset_got_prefix.release();
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::H && !!(ev->modifiers() & Modifiers::Control)) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                auto layout_entry = tab.layout_tree->find_pane(tab.active);
                                if (!layout_entry) {
                                    return;
                                }

                                // Handle wrap.
                                auto col = layout_entry->col <= 1 ? state.size.cols - 1 : layout_entry->col - 2;

                                auto candidates =
                                    tab.layout_tree->hit_test_vertical_line(
                                        col, layout_entry->row, layout_entry->row + layout_entry->size.rows) |
                                    di::transform(&LayoutEntry::pane) | di::to<di::TreeSet>();

                                for (auto* candidate : tab.panes_ordered_by_recency) {
                                    if (candidate != tab.active && candidates.contains(candidate)) {
                                        set_active(state, tab, candidate);
                                        break;
                                    }
                                }
                                reset_got_prefix.release();
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::L && !!(ev->modifiers() & Modifiers::Control)) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                auto layout_entry = tab.layout_tree->find_pane(tab.active);
                                if (!layout_entry) {
                                    return;
                                }

                                // Handle wrap.
                                auto col = (state.size.cols < 2 ||
                                            layout_entry->col + layout_entry->size.cols >= state.size.cols - 2)
                                               ? 0
                                               : layout_entry->col + layout_entry->size.cols + 1;

                                auto candidates =
                                    tab.layout_tree->hit_test_vertical_line(
                                        col, layout_entry->row, layout_entry->row + layout_entry->size.rows) |
                                    di::transform(&LayoutEntry::pane) | di::to<di::TreeSet>();

                                for (auto* candidate : tab.panes_ordered_by_recency) {
                                    if (candidate != tab.active && candidates.contains(candidate)) {
                                        set_active(state, tab, candidate);
                                        break;
                                    }
                                }
                                reset_got_prefix.release();
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::K && !!(ev->modifiers() & Modifiers::Control)) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                auto layout_entry = tab.layout_tree->find_pane(tab.active);
                                if (!layout_entry) {
                                    return;
                                }

                                // Handle wrap.
                                auto row = layout_entry->row <= 1 ? state.size.rows - 1 : layout_entry->row - 2;

                                auto candidates =
                                    tab.layout_tree->hit_test_horizontal_line(
                                        row, layout_entry->col, layout_entry->col + layout_entry->size.cols) |
                                    di::transform(&LayoutEntry::pane) | di::to<di::TreeSet>();

                                for (auto* candidate : tab.panes_ordered_by_recency) {
                                    if (candidate != tab.active && candidates.contains(candidate)) {
                                        set_active(state, tab, candidate);
                                        break;
                                    }
                                }
                                reset_got_prefix.release();
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::J && !!(ev->modifiers() & Modifiers::Control)) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                auto layout_entry = tab.layout_tree->find_pane(tab.active);
                                if (!layout_entry) {
                                    return;
                                }

                                // Handle wrap.
                                auto row = (state.size.rows < 2 ||
                                            layout_entry->row + layout_entry->size.rows >= state.size.rows - 2)
                                               ? 0
                                               : layout_entry->row + layout_entry->size.rows + 1;

                                auto candidates =
                                    tab.layout_tree->hit_test_horizontal_line(
                                        row, layout_entry->col, layout_entry->col + layout_entry->size.cols) |
                                    di::transform(&LayoutEntry::pane) | di::to<di::TreeSet>();

                                for (auto* candidate : tab.panes_ordered_by_recency) {
                                    if (candidate != tab.active && candidates.contains(candidate)) {
                                        set_active(state, tab, candidate);
                                        break;
                                    }
                                }
                                reset_got_prefix.release();
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::C) {
                            layout_state.with_lock([&](LayoutState& state) {
                                (void) add_tab(state, di::clone(args.command));
                            });
                            continue;
                        }

                        auto window_nav_keys =
                            di::range(i32(Key::_1), i32(Key::_9) + 1) | di::transform(di::construct<Key>);
                        if (got_prefix && di::contains(window_nav_keys, ev->key())) {
                            layout_state.with_lock([&](LayoutState& state) {
                                auto index = usize(ev->key()) - usize(Key::_1);
                                if (auto tab = state.tabs.at(index)) {
                                    set_active_tab(state, tab->get());
                                }
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::D) {
                            set_done();
                            break;
                        }

                        if (got_prefix && ev->key() == Key::X) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                if (auto* pane = tab.active) {
                                    pane->exit();
                                }
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::BackSlash && !!(Modifiers::Shift)) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                (void) add_pane(state, tab, di::clone(args.command), Direction::Horizontal);
                            });
                            continue;
                        }

                        if (got_prefix && ev->key() == Key::Minus) {
                            layout_state.with_lock([&](LayoutState& state) {
                                if (!state.active_tab) {
                                    return;
                                }
                                auto& tab = *state.active_tab;
                                (void) add_pane(state, tab, di::clone(args.command), Direction::Vertical);
                            });
                            continue;
                        }
                    }

                    // NOTE: we need to hold the layout state lock the entire time
                    // to prevent the Pane object from being prematurely destroyed.
                    layout_state.with_lock([&](LayoutState& state) {
                        if (!state.active_tab) {
                            return;
                        }
                        auto& tab = *state.active_tab;
                        if (auto* pane = tab.active) {
                            pane->event(*ev);
                        }
                    });
                } else if (auto ev = di::get_if<MouseEvent>(event)) {
                    layout_state.with_lock([&](LayoutState& state) {
                        if (!state.active_tab) {
                            return;
                        }
                        auto& tab = *state.active_tab;

                        // Check if the event interests with any pane.
                        for (auto const& entry :
                             tab.layout_tree->hit_test(ev->position().in_cells().y(), ev->position().in_cells().x())) {
                            if (ev->type() != MouseEventType::Move) {
                                set_active(state, tab, entry.pane);
                            }
                            if (entry.pane == tab.active) {
                                entry.pane->event(ev->translate({ -entry.col, -entry.row }, state.size));
                            }
                        }
                    });
                } else if (auto ev = di::get_if<FocusEvent>(event)) {
                    layout_state.with_lock([&](LayoutState& state) {
                        if (!state.active_tab) {
                            return;
                        }
                        auto& tab = *state.active_tab;
                        if (auto* pane = tab.active) {
                            pane->event(*ev);
                        }
                    });
                } else if (auto ev = di::get_if<PasteEvent>(event)) {
                    layout_state.with_lock([&](LayoutState& state) {
                        if (!state.active_tab) {
                            return;
                        }
                        auto& tab = *state.active_tab;
                        if (auto* pane = tab.active) {
                            pane->event(*ev);
                        }
                    });
                }
            }
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) input_thread.join();
    });

    auto render_thread = TRY(dius::Thread::create([&] -> void {
        auto renderer = Renderer();

        auto deadline = dius::SteadyClock::now();
        while (!done.load(di::MemoryOrder::Acquire)) {
            auto [did_render, cursor] =
                layout_state.with_lock([&](LayoutState& state) -> di::Tuple<bool, di::Optional<RenderedCursor>> {
                    // Process any pending events.
                    for (auto event : state.events) {
                        di::visit(di::overload(
                                      [&](dius::tty::WindowSize const& size) {
                                          do_layout(state, size);
                                      },
                                      [&](PaneExited const& pane) {
                                          remove_pane(state, *pane.tab, pane.pane);
                                      }),
                                  event);
                    }

                    // Ignore if there is no layout.
                    if (!state.active_tab) {
                        return { false, {} };
                    }
                    auto& tab = *state.active_tab;
                    if (!tab.layout_tree) {
                        return { false, {} };
                    }

                    // Do the render.
                    renderer.start(state.size);

                    // Status bar.
                    auto text = di::enumerate(state.tabs) |
                                di::transform(di::uncurry([&](usize i, di::Box<Tab> const& tab) {
                                    return *di::present("[{}{} {}]"_sv, tab.get() == state.active_tab ? U'*' : U' ',
                                                        i + 1, tab->name);
                                })) |
                                di::join_with(U' ') | di::to<di::String>();
                    renderer.clear_row(0);
                    renderer.put_text(text.view(), 0, 0);

                    auto cursor = di::Optional<RenderedCursor> {};

                    struct PositionAndSize {
                        static auto operator()(di::Box<LayoutNode> const& node)
                            -> di::Tuple<u32, u32, dius::tty::WindowSize> {
                            return { node->row, node->col, node->size };
                        }

                        static auto operator()(LayoutEntry const& entry) -> di::Tuple<u32, u32, dius::tty::WindowSize> {
                            return { entry.row, entry.col, entry.size };
                        }
                    };

                    struct Render {
                        Renderer& renderer;
                        di::Optional<RenderedCursor>& cursor;
                        Tab& tab;
                        LayoutState& state;

                        void operator()(di::Box<LayoutNode> const& node) {
                            auto first = true;
                            for (auto const& child : node->children) {
                                if (!first) {
                                    // Draw a border around the pane.
                                    auto [row, col, size] = di::visit(PositionAndSize {}, child);
                                    renderer.set_bound(0, 0, state.size.cols, state.size.rows);
                                    if (node->direction == Direction::Horizontal) {
                                        for (auto r : di::range(row, row + size.rows)) {
                                            auto code_point = U'│';
                                            renderer.put_text(code_point, r, col - 1);
                                        }
                                    } else if (node->direction == Direction::Vertical) {
                                        for (auto c : di::range(col, col + size.cols)) {
                                            auto code_point = U'─';
                                            renderer.put_text(code_point, row - 1, c);
                                        }
                                    }
                                }
                                first = false;

                                di::visit(*this, child);
                            }
                        }

                        void operator()(LayoutEntry const& entry) {
                            renderer.set_bound(entry.row, entry.col, entry.size.cols, entry.size.rows);
                            auto pane_cursor = entry.pane->draw(renderer);
                            if (entry.pane == tab.active) {
                                pane_cursor.cursor_row += entry.row;
                                pane_cursor.cursor_col += entry.col;
                                cursor = pane_cursor;
                            }
                        }
                    };

                    Render(renderer, cursor, tab, state)(tab.layout_tree);

                    return { true, cursor };
                });

            if (did_render) {
                (void) renderer.finish(dius::stdin, cursor.value_or({ .hidden = true }));
            }

            while (deadline < dius::SteadyClock::now()) {
                deadline += di::Milliseconds(25); // 50 FPS
            }
            dius::this_thread::sleep_until(deadline);
        }
    }));
    auto _ = di::ScopeExit([&] {
        (void) render_thread.join();
    });

    while (!done.load(di::MemoryOrder::Acquire)) {
        if (!dius::system::wait_for_signal(dius::Signal::WindowChange)) {
            break;
        }
        if (done.load(di::MemoryOrder::Acquire)) {
            break;
        }

        auto size = dius::stdin.get_tty_window_size();
        if (!size) {
            continue;
        }

        layout_state.with_lock([&](LayoutState& state) {
            state.events.push(size.value());
        });
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
