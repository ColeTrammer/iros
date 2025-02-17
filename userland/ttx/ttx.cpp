#include "di/cli/parser.h"
#include "di/container/interface/erase.h"
#include "di/container/string/string_view.h"
#include "di/function/not_equal.h"
#include "di/io/writer_print.h"
#include "di/math/rational/rational.h"
#include "di/sync/synchronized.h"
#include "dius/main.h"
#include "dius/print.h"
#include "dius/sync_file.h"
#include "dius/system/process.h"
#include "dius/thread.h"
#include "dius/tty.h"
#include "pane.h"
#include "ttx/focus_event.h"
#include "ttx/terminal_input.h"
#include "ttx/utf8_stream_decoder.h"

namespace ttx {
struct Args {
    di::Vector<di::TransparentStringView> command;
    usize panes { 1 };
    bool help { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("ttx"_sv, "Terminal multiplexer"_sv)
            .option<&Args::panes>('p', "panes"_tsv, "Number of panes to start with"_sv)
            .argument<&Args::command>("COMMAND"_sv, "Program to run in terminal"_sv, true)
            .help();
    }
};

struct LayoutEntry {
    u32 row { 0 };
    u32 col { 0 };
    dius::tty::WindowSize size;
    Pane* pane { nullptr };
};

struct LayoutState {
    di::Vector<di::Box<Pane>> panes {};
    di::Vector<LayoutEntry> layout {};
    Pane* active { nullptr };
    dius::tty::WindowSize size;
};

static auto main(Args& args) -> di::Result<void> {
    if (args.panes == 0) {
        dius::eprintln("error: ttx needs to start with at least one pane"_sv);
        return {};
    }

    auto done = di::Atomic<bool>(false);

    auto set_done = [&] {
        // TODO: send kill signals (SIGHUP) to all remaining panes.
        // TODO: timeout/skip waiting for processes to die after sending SIGHUP.
        done.store(true, di::MemoryOrder::Release);
        // Ensure the SIGWINCH thread exits.
        (void) dius::system::send_signal(dius::system::get_process_id(), dius::Signal::WindowChange);
        // Ensure the input thread exits. (By writing something to stdin).
        auto byte = 0_b;
        (void) dius::stdin.write_exactly({ &byte, 1 });
    };

    auto layout_state = di::Synchronized(LayoutState {
        .size = TRY(dius::stdin.get_tty_window_size()),
    });

    auto do_layout = [&](LayoutState& state, dius::tty::WindowSize size) {
        state.layout = {};
        state.size = size;

        if (size.rows <= 1 || state.panes.empty() || size.cols <= state.panes.size() - 1) {
            return;
        }

        // Allocate the first row to the status bar.
        auto available_size = size;
        available_size.pixel_height -= size.pixel_height / size.rows;
        available_size.rows--;

        // Allocate 1 column for padding in between panes.
        available_size.pixel_width -= (state.panes.size() - 1) * size.pixel_width / size.cols;
        available_size.cols -= state.panes.size() - 1;

        auto target_cols = di::Rational(i32(size.cols), i32(state.panes.size()));
        auto target_xpixels = di::Rational(i32(size.pixel_width), i32(state.panes.size()));
        auto leftover_cols = di::Rational(i32(0));
        auto leftover_xpixels = di::Rational(i32(0));
        auto x = 0_u32;
        for (auto const& pane : state.panes) {
            leftover_cols += target_cols;
            leftover_xpixels += target_xpixels;

            auto cols = leftover_cols.round();
            auto xpixels = leftover_xpixels.round();

            auto size =
                dius::tty::WindowSize { available_size.rows, (u32) cols, (u32) xpixels, available_size.pixel_height };
            state.layout.push_back({
                1,
                x,
                size,
                pane.get(),
            });

            // NOTE: pane is null when determining what the initial size for the pane should be.
            if (pane) {
                pane->resize(size);
            }

            leftover_cols -= cols;
            leftover_xpixels -= xpixels;
            x += cols;

            // Account for horizontal padding.
            x++;
        }
    };

    auto set_active = [&](LayoutState& state, Pane* pane) {
        if (state.active == pane) {
            return;
        }

        // Unfocus the old pane, and focus the new pane.
        if (state.active) {
            state.active->event(FocusEvent::focus_out());
        }
        state.active = pane;
        if (state.active) {
            state.active->event(FocusEvent::focus_in());
        }
    };

    auto remove_pane = [&](Pane* pane) {
        layout_state.with_lock([&](LayoutState& state) {
            // Clear active pane.
            if (state.active == pane) {
                // TODO: use a better algorithm. Last used?
                auto candidates = state.panes | di::transform(&di::Box<Pane>::get) | di::filter(di::not_equal(pane));
                set_active(state, candidates.front().value_or(nullptr));
            }

            di::erase_if(state.panes, [&](di::Box<Pane> const& pointer) {
                return pointer.get() == pane;
            });
            do_layout(state, state.size);

            // Exit when there are no panes left.
            if (state.panes.empty()) {
                set_done();
            }
        });
        return;
    };

    auto add_pane = [&](di::Vector<di::TransparentStringView> command) -> di::Result<> {
        return layout_state.with_lock([&](LayoutState& state) -> di::Result<> {
            // Dummy state for initial layout.
            state.panes.push_back(nullptr);

            do_layout(state, state.size);

            auto node = state.layout.back();
            if (!node || node.value().pane != nullptr) {
                // NOTE: this happens when the visible terminal size is too small.
                state.panes.pop_back();
                do_layout(state, state.size);
                return di::Unexpected(di::BasicError::InvalidArgument);
            }

            auto maybe_pane = Pane::create(di::move(command), state.layout.back().value().size);
            if (!maybe_pane) {
                state.panes.pop_back();
                do_layout(state, state.size);
                return di::Unexpected(di::move(maybe_pane).error());
            }

            auto& pane = state.panes.back().value() = di::move(maybe_pane).value();
            state.layout.back().value().pane = pane.get();
            pane->did_exit = [&remove_pane, pane = pane.get()] {
                remove_pane(pane);
            };

            set_active(state, pane.get());
            return {};
        });
    };

    // Initial panes.
    for (auto _ : di::range(args.panes)) {
        TRY(add_pane(di::clone(args.command)));
    }

    // Setup - raw mode
    auto _ = TRY(dius::stdin.enter_raw_mode());

    // Setup - alternate screen buffer.
    di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049h\033[H\033[2J"_sv);
    auto _ = di::ScopeExit([&] {
        di::writer_print<di::String::Encoding>(dius::stdin, "\033[?1049l"_sv);
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

    [[maybe_unused]] auto& log = dius::stderr = TRY(dius::open_sync("/tmp/ttx.log"_pv, dius::OpenMode::WriteClobber));

    TRY(dius::system::mask_signal(dius::Signal::WindowChange));

    auto input_thread = TRY(dius::Thread::create([&] -> void {
        auto _ = di::ScopeExit([&] {
            set_done();
        });

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
                    if (ev->key() == Key::Q && !!(ev->modifiers() & Modifiers::Control)) {
                        set_done();
                        break;
                    }

                    if (ev->type() == KeyEventType::Press && ev->key() == Key::H &&
                        !!(ev->modifiers() & Modifiers::Control)) {
                        layout_state.with_lock([&](LayoutState& state) {
                            auto cycled = di::cycle(state.panes);
                            auto it = di::find(cycled, state.active, &di::Box<Pane>::get);
                            if (it != cycled.end()) {
                                --it;
                                set_active(state, (*it).get());
                            }
                        });
                        continue;
                    }

                    if (ev->type() == KeyEventType::Press && ev->key() == Key::L &&
                        !!(ev->modifiers() & Modifiers::Control)) {
                        layout_state.with_lock([&](LayoutState& state) {
                            auto cycled = di::cycle(state.panes);
                            auto it = di::find(cycled, state.active, &di::Box<Pane>::get);
                            if (it != cycled.end()) {
                                ++it;
                                set_active(state, (*it).get());
                            }
                        });
                        continue;
                    }

                    // NOTE: we need to hold the layout state lock the entire time
                    // to prevent the Pane object from being prematurely destroyed.
                    layout_state.with_lock([&](LayoutState& state) {
                        if (auto* pane = state.active) {
                            pane->event(*ev);
                        }
                    });
                } else if (auto ev = di::get_if<MouseEvent>(event)) {
                    // TODO: change focus, simluate focus events, and validate mouse coordinates are in bounds.
                    // TODO: translate coordinates.
                    layout_state.with_lock([&](LayoutState& state) {
                        // Check if the event interests with any pane.
                        for (auto const& entry : state.layout) {
                            if (ev->position().in_cells().x() < entry.col ||
                                ev->position().in_cells().y() < entry.row ||
                                ev->position().in_cells().x() >= entry.col + entry.size.cols ||
                                ev->position().in_cells().y() >= entry.row + entry.size.rows) {
                                continue;
                            }

                            if (ev->type() != MouseEventType::Move) {
                                set_active(state, entry.pane);
                            }
                            if (entry.pane == state.active) {
                                entry.pane->event(ev->translate({ -entry.col, -entry.row }, state.size));
                            }
                        }
                    });
                } else if (auto ev = di::get_if<FocusEvent>(event)) {
                    layout_state.with_lock([&](LayoutState& state) {
                        if (auto* pane = state.active) {
                            pane->event(*ev);
                        }
                    });
                } else if (auto ev = di::get_if<PasteEvent>(event)) {
                    layout_state.with_lock([&](LayoutState& state) {
                        if (auto* pane = state.active) {
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
            auto cursor = layout_state.with_lock([&](LayoutState& state) {
                renderer.start(state.size);

                // Status bar.
                static int i = 0;
                renderer.put_text(di::present("[{}]"_sv, ++i)->view(), 0, 0,
                                  GraphicsRendition {
                                      .font_weight = FontWeight::Bold,
                                      .inverted = true,
                                  });

                auto cursor = di::Optional<RenderedCursor> {};
                for (auto [i, entry] : di::enumerate(state.layout)) {
                    // Draw horizontal line between panes.
                    renderer.set_bound(0, 0, state.size.cols, state.size.rows);
                    for (auto row : di::range(entry.row, entry.row + entry.size.rows)) {
                        renderer.put_text(U'│', row, entry.col - 1);
                    }

                    renderer.set_bound(entry.row, entry.col, entry.size.cols, entry.size.rows);
                    auto pane_cursor = entry.pane->draw(renderer);
                    if (entry.pane == state.active) {
                        pane_cursor.cursor_row += entry.row;
                        pane_cursor.cursor_col += entry.col;
                        cursor = pane_cursor;
                    }
                }
                return cursor;
            });

            (void) renderer.finish(dius::stdin, cursor.value_or({ .hidden = true }));

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
            do_layout(state, size.value());
        });
    }

    return {};
}
}

DIUS_MAIN(ttx::Args, ttx)
