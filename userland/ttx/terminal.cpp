#include "terminal.h"

#include "di/container/algorithm/contains.h"
#include "di/container/algorithm/rotate.h"
#include "escape_sequence_parser.h"

namespace ttx {
void Terminal::on_parser_results(di::Span<ParserResult const> results) {
    for (auto const& result : results) {
        di::visit(
            [&](auto const& r) {
                this->on_parser_result(r);
            },
            result);
    }
}

void Terminal::on_parser_result(PrintableCharacter const& printable_character) {
    if (printable_character.code_point < 0x7F || printable_character.code_point > 0x9F) {
        put_char(printable_character.code_point);
    }
}

void Terminal::on_parser_result(ControlCharacter const& control_character) {
    switch (control_character.code_point) {
        case 8:
            return c0_bs();
        case '\a':
            return;
        case '\t':
            return c0_ht();
        case '\n':
            return c0_lf();
        case '\v':
            return c0_vt();
        case '\f':
            return c0_ff();
        case '\r':
            return c0_cr();
    }
}

void Terminal::on_parser_result(CSI const& csi) {
    if (csi.intermediate == "="_sv) {
        switch (csi.terminator) {
            case 'c':
                return csi_da3(csi.params);
        }
        return;
    }

    if (csi.intermediate == ">"_sv) {
        switch (csi.terminator) {
            case 'c':
                return csi_da2(csi.params);
        }
        return;
    }

    if (csi.intermediate == "?"_sv) {
        switch (csi.terminator) {
            case 'h':
                return csi_decset(csi.params);
            case 'l':
                return csi_decrst(csi.params);
        }
        return;
    }

    if (!csi.intermediate.empty()) {
        return;
    }

    switch (csi.terminator) {
        case '@':
            return csi_ich(csi.params);
        case 'A':
            return csi_cuu(csi.params);
        case 'B':
            return csi_cud(csi.params);
        case 'C':
            return csi_cuf(csi.params);
        case 'D':
            return csi_cub(csi.params);
        case 'G':
            return csi_cha(csi.params);
        case 'H':
            return csi_cup(csi.params);
        case 'J':
            return csi_ed(csi.params);
        case 'K':
            return csi_el(csi.params);
        case 'L':
            return csi_il(csi.params);
        case 'M':
            return csi_dl(csi.params);
        case 'P':
            return csi_dch(csi.params);
        case 'S':
            return csi_su(csi.params);
        case 'T':
            return csi_sd(csi.params);
        case 'X':
            return csi_ech(csi.params);
        case 'b':
            return csi_rep(csi.params);
        case 'c':
            return csi_da1(csi.params);
        case 'd':
            return csi_vpa(csi.params);
        case 'f':
            return csi_hvp(csi.params);
        case 'g':
            return csi_tbc(csi.params);
        case 'm':
            return csi_sgr(csi.params);
        case 'n':
            return csi_dsr(csi.params);
        case 'r':
            return csi_decstbm(csi.params);
        case 's':
            return csi_scosc(csi.params);
        case 'u':
            return csi_scorc(csi.params);
    }
    return;
}

void Terminal::on_parser_result(Escape const& escape) {
    if (escape.intermediate == "#"_sv) {
        switch (escape.terminator) {
            case '8':
                return esc_decaln();
        }
        return;
    }

    if (!escape.intermediate.empty()) {
        return;
    }

    switch (escape.terminator) {
        case '7':
            return esc_decsc();
        case '8':
            return esc_decrc();
    }

    // 8 bit control characters
    switch (escape.terminator) {
        case 'D':
            return c1_ind();
        case 'E':
            return c1_nel();
        case 'H':
            return c1_hts();
        case 'M':
            return c1_ri();
    }
}

// Backspace - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_bs() {
    if (m_cursor_col) {
        m_cursor_col--;
    }
    m_x_overflow = false;
}

// Horizontal Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_ht() {
    for (auto tab_stop : m_tab_stops) {
        if (tab_stop > m_cursor_col) {
            return set_cursor(m_cursor_row, tab_stop);
        }
    }

    set_cursor(m_cursor_row, max_col_inclusive());
}

// Line Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_lf() {
    m_cursor_row++;
    scroll_down_if_needed();
    m_x_overflow = false;
}

// Vertical Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_vt() {
    c0_lf();
}

// Form Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_ff() {
    c0_lf();
}

// Carriage Return - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void Terminal::c0_cr() {
    m_cursor_col = 0;
    m_x_overflow = false;
}

// Index - https://vt100.net/docs/vt510-rm/IND.html
void Terminal::c1_ind() {
    m_cursor_row++;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Next Line - https://vt100.net/docs/vt510-rm/NEL.html
void Terminal::c1_nel() {
    m_cursor_row++;
    m_cursor_col = 0;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Horizontal Tab Set - https://vt100.net/docs/vt510-rm/HTS.html
void Terminal::c1_hts() {
    if (di::contains(m_tab_stops, m_cursor_col)) {
        return;
    }

    auto index = 0_usize;
    for (; index < m_tab_stops.size(); index++) {
        if (m_cursor_col < m_tab_stops[index]) {
            break;
        }
    }

    m_tab_stops.insert(m_tab_stops.begin() + index, m_cursor_col);
}

// Reverse Index - https://www.vt100.net/docs/vt100-ug/chapter3.html#RI
void Terminal::c1_ri() {
    m_cursor_row--;
    m_x_overflow = false;
    scroll_up_if_needed();
}

// DEC Screen Alignment Pattern - https://vt100.net/docs/vt510-rm/DECALN.html
void Terminal::esc_decaln() {
    clear('E');
    set_cursor(0, 0);
    m_x_overflow = false;
}

// DEC Save Cursor - https://vt100.net/docs/vt510-rm/DECSC.html
void Terminal::esc_decsc() {
    save_pos();
}

// DEC Restore Cursor - https://vt100.net/docs/vt510-rm/DECRC.html
void Terminal::esc_decrc() {
    restore_pos();
}

// Insert Character - https://vt100.net/docs/vt510-rm/ICH.html
void Terminal::csi_ich(di::Vector<int> const& params) {
    auto chars = di::max(1, params.at(0).value_or(1));

    auto& row = m_rows[m_cursor_row];
    for (int i = max_col_inclusive() - chars; i >= m_cursor_col; i--) {
        row[i + chars] = row[i];
    }

    for (int i = m_cursor_col; i <= max_col_inclusive() && i < m_cursor_col + chars; i++) {
        row[i] = {};
    }
}

// Cursor Up - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUU
void Terminal::csi_cuu(di::Vector<int> const& params) {
    auto delta_row = di::max(1, params.at(0).value_or(1));
    set_cursor(m_cursor_row - delta_row, m_cursor_col);
}

// Cursor Down - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUD
void Terminal::csi_cud(di::Vector<int> const& params) {
    auto delta_row = di::max(1, params.at(0).value_or(1));
    set_cursor(m_cursor_row + delta_row, m_cursor_col);
}

// Cursor Forward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUF
void Terminal::csi_cuf(di::Vector<int> const& params) {
    auto delta_col = di::max(1, params.at(0).value_or(1));
    set_cursor(m_cursor_row, m_cursor_col + delta_col);
}

// Cursor Backward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUB
void Terminal::csi_cub(di::Vector<int> const& params) {
    auto delta_col = di::max(1, params.at(0).value_or(1));
    set_cursor(m_cursor_row, m_cursor_col - delta_col);
}

// Cursor Position - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUP
void Terminal::csi_cup(di::Vector<int> const& params) {
    auto row = translate_row(params.at(0).value_or(1));
    auto col = translate_col(params.at(1).value_or(1));
    set_cursor(row, col);
}

// Cursor Horizontal Absolute - https://vt100.net/docs/vt510-rm/CHA.html
void Terminal::csi_cha(di::Vector<int> const& params) {
    set_cursor(m_cursor_row, translate_col(params.at(0).value_or(1)));
}

// Erase in Display - https://vt100.net/docs/vt510-rm/ED.html
void Terminal::csi_ed(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 0:
            return clear_below_cursor();
        case 1:
            return clear_above_cursor();
        case 2:
            return clear();
        case 3:
            // XTerm extension to clear scoll buffer
            m_rows_above.clear();
            m_rows_below.clear();
            m_rows.resize(m_row_count);
            clear();
            return;
    }
}

// Erase in Line - https://vt100.net/docs/vt510-rm/EL.html
void Terminal::csi_el(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 0:
            return clear_row_to_end(m_cursor_row, m_cursor_col);
        case 1:
            return clear_row_until(m_cursor_row, m_cursor_col);
        case 2:
            return clear_row(m_cursor_row);
    }
}

// Insert Line - https://vt100.net/docs/vt510-rm/IL.html
void Terminal::csi_il(di::Vector<int> const& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    int lines_to_insert = di::max(1, params.at(0).value_or(1));
    for (int i = 0; i < lines_to_insert; i++) {
        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
        // m_rows.rotate_right(m_cursor_row, m_scroll_end + 1);
        m_rows[m_cursor_row] = Row();
        m_rows[m_cursor_row].resize(m_col_count);
    }
    invalidate_all();
}

// Delete Line - https://vt100.net/docs/vt510-rm/DL.html
void Terminal::csi_dl(di::Vector<int> const& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    int lines_to_delete = di::clamp(params.at(0).value_or(1), 1, m_scroll_end - m_cursor_row);
    for (int i = 0; i < lines_to_delete; i++) {
        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_cursor_row + 1, m_rows.begin() + m_scroll_end + 1);
        // m_rows.rotate_left(m_cursor_row, m_scroll_end + 1);
        m_rows[m_scroll_end] = Row();
        m_rows[m_scroll_end].resize(m_col_count);
    }

    invalidate_all();
}

// Delete Character - https://vt100.net/docs/vt510-rm/DCH.html
void Terminal::csi_dch(di::Vector<int> const& params) {
    int chars_to_delete = di::clamp(params.at(0).value_or(1), 1, m_col_count - m_cursor_col);
    for (int i = 0; i < chars_to_delete; i++) {
        m_rows[m_cursor_row].erase(m_rows[m_cursor_row].begin() + m_cursor_col);
    }
    m_rows[m_cursor_row].resize(m_col_count);
    for (int i = m_cursor_col; i < m_col_count; i++) {
        m_rows[m_cursor_row][i].dirty = true;
    }
}

// Pan Down - https://vt100.net/docs/vt510-rm/SU.html
void Terminal::csi_su(di::Vector<int> const& params) {
    int to_scroll = params.at(0).value_or(1);
    int row_save = m_cursor_row;
    for (int i = 0; i < to_scroll; i++) {
        m_cursor_row = m_row_count;
        scroll_down_if_needed();
    }
    m_cursor_row = row_save;
    return;
}

// Pan Up - https://vt100.net/docs/vt510-rm/SD.html
void Terminal::csi_sd(di::Vector<int> const& params) {
    int to_scroll = params.at(0).value_or(1);
    int row_save = m_cursor_row;
    for (int i = 0; i < to_scroll; i++) {
        m_cursor_row = -1;
        scroll_up_if_needed();
    }
    m_cursor_row = row_save;
}

// Erase Character - https://vt100.net/docs/vt510-rm/ECH.html
void Terminal::csi_ech(di::Vector<int> const& params) {
    int chars_to_erase = di::max(1, params.at(0).value_or(1));
    for (int i = m_cursor_col; i - m_cursor_col < chars_to_erase && i < m_col_count; i++) {
        m_rows[m_cursor_row][i] = {};
    }
}

// Repeat Preceding Graphic Character - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_rep(di::Vector<int> const& params) {
    c32 preceding_character = ' ';
    if (m_cursor_col == 0) {
        if (m_cursor_row != 0) {
            preceding_character = m_rows[m_cursor_row - 1][m_col_count - 1].ch;
        }
    } else {
        preceding_character = m_rows[m_cursor_row][m_cursor_col - 1].ch;
    }
    for (int i = 0; i < params.at(0).value_or(0); i++) {
        put_char(preceding_character);
    }
}

// Primary Device Attributes - https://vt100.net/docs/vt510-rm/DA1.html
void Terminal::csi_da1(di::Vector<int> const& params) {
    if (params.at(0).value_or(0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[?1;0c"_sv.span()));
}

// Secondary Device Attributes - https://vt100.net/docs/vt510-rm/DA2.html
void Terminal::csi_da2(di::Vector<int> const& params) {
    if (params.at(0).value_or(0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[>010;0c"_sv.span()));
}

// Tertiary Device Attributes - https://vt100.net/docs/vt510-rm/DA3.html
void Terminal::csi_da3(di::Vector<int> const& params) {
    if (params.at(0).value_or(0) != 0) {
        return;
    }
    (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033P!|00000000\033\\"_sv.span()));
}

// Vertical Line Position Absolute - https://vt100.net/docs/vt510-rm/VPA.html
void Terminal::csi_vpa(di::Vector<int> const& params) {
    set_cursor(translate_row(params.at(0).value_or(1)), m_cursor_col);
}

// Horizontal and Vertical Position - https://vt100.net/docs/vt510-rm/HVP.html
void Terminal::csi_hvp(di::Vector<int> const& params) {
    csi_cup(params);
}

// Tab Clear - https://vt100.net/docs/vt510-rm/TBC.html
void Terminal::csi_tbc(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 0:
            di::erase_if(m_tab_stops, [this](auto x) {
                return x == m_cursor_col;
            });
            return;
        case 3:
            m_tab_stops.clear();
            return;
    }
}

// DEC Private Mode Set - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_decset(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            // m_psuedo_terminal.set_application_cursor_keys(true);
            break;
        case 3:
            // Select 80 or 132 Columns per Page - https://vt100.net/docs/vt510-rm/DECCOLM.html
            if (m_allow_80_132_col_mode) {
                m_80_col_mode = false;
                m_132_col_mode = true;
                resize(m_row_count, 132);
                clear();
                csi_decstbm({});
            }
            break;
        case 6:
            // Origin Mode - https://vt100.net/docs/vt510-rm/DECOM.html
            m_origin_mode = true;
            set_cursor(m_cursor_row, m_cursor_col);
            break;
        case 7:
            // Autowrap mode - https://vt100.net/docs/vt510-rm/DECAWM.html
            m_autowrap_mode = true;
            break;
        case 9:
            // m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
            // m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
            break;
        case 25:
            // Text Cursor Enable Mode - https://vt100.net/docs/vt510-rm/DECTCEM.html
            m_cursor_hidden = false;
            break;
        case 40:
            m_allow_80_132_col_mode = true;
            break;
        case 1000:
            // m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
            // m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
            break;
        case 1001:
            // m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X11);
            // m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1002:
            // m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::Cell);
            // m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1003:
            // m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::All);
            // m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1005:
            // if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::UTF8);
            // }
            break;
        case 1006:
            // if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::SGR);
            // }
            break;
        case 1015:
            // if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
            //     m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::URXVT);
            // }
            break;
        case 1049:
            set_use_alternate_screen_buffer(true);
            break;
        case 2004:
            // m_psuedo_terminal.set_bracketed_paste(true);
            break;
        default:
            break;
    }
}

// DEC Private Mode Reset - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void Terminal::csi_decrst(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            // m_psuedo_terminal.set_application_cursor_keys(false);
            break;
        case 3:
            // Select 80 or 132 Columns per Page - https://vt100.net/docs/vt510-rm/DECCOLM.html
            if (m_allow_80_132_col_mode) {
                m_80_col_mode = true;
                m_132_col_mode = false;
                resize(m_row_count, 80);
                clear();
                csi_decstbm({});
            }
            break;
        case 6:
            // Origin Mode - https://vt100.net/docs/vt510-rm/DECOM.html
            m_origin_mode = false;
            break;
        case 7:
            // Autowrap mode - https://vt100.net/docs/vt510-rm/DECAWM.html
            m_autowrap_mode = false;
            break;
        case 9:
            // m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X10);
            break;
        case 25:
            // Text Cursor Enable Mode - https://vt100.net/docs/vt510-rm/DECTCEM.html
            m_cursor_hidden = true;
            break;
        case 40:
            m_allow_80_132_col_mode = false;
            if (m_80_col_mode || m_132_col_mode) {
                m_80_col_mode = false;
                m_132_col_mode = false;
                resize(m_available_rows_in_display, m_available_cols_in_display);
            }
            break;
        case 1000:
            // m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X11);
            break;
        case 1001:
            // m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Hilite);
            break;
        case 1002:
            // m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Cell);
            break;
        case 1003:
            // m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::All);
            break;
        case 1005:
            // if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::UTF8) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            // }
            break;
        case 1006:
            // if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::SGR) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            // }
            break;
        case 1015:
            // if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::URXVT) {
            //     m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            // }
            break;
        case 1049:
            set_use_alternate_screen_buffer(false);
            break;
        case 2004:
            // m_psuedo_terminal.set_bracketed_paste(false);
            break;
        default:
            break;
    }
}

// Select Graphics Rendition - https://vt100.net/docs/vt510-rm/SGR.html
void Terminal::csi_sgr(di::Vector<int> const& params) {
    for (auto i = 0_usize; i == 0 || i < params.size(); i++) {
        switch (params.at(i).value_or(0)) {
            case 0:
                reset_attributes();
                break;
            case 1:
                set_bold(true);
                break;
            case 2:
                set_dim(true);
                break;
            case 3:
                set_italic(true);
                break;
            case 4:
                set_underline(true);
                break;
            case 5:
                set_blink(true);
                break;
            case 6:
                set_rapid_blink(true);
                break;
            case 7:
                set_inverted(true);
                break;
            case 8:
                set_invisible(true);
                break;
            case 9:
                set_strike_through(true);
                break;
            case 21:
                set_underline(true);
                break;
            case 22:
                set_bold(false);
                set_dim(false);
                break;
            case 23:
                set_italic(false);
                break;
            case 24:
                set_underline(false);
                break;
            case 25:
                set_blink(false);
                set_rapid_blink(false);
                break;
            case 27:
                set_inverted(false);
                break;
            case 28:
                set_invisible(false);
                break;
            case 29:
                set_strike_through(false);
                break;
            case 30:
                set_fg({ Color::Palette::Black });
                break;
            case 31:
                set_fg({ Color::Palette::Red });
                break;
            case 32:
                set_fg({ Color::Palette::Green });
                break;
            case 33:
                set_fg({ Color::Palette::Brown });
                break;
            case 34:
                set_fg({ Color::Palette::Blue });
                break;
            case 35:
                set_fg({ Color::Palette::Magenta });
                break;
            case 36:
                set_fg({ Color::Palette::Cyan });
                break;
            case 37:
                set_fg({ Color::Palette::LightGrey });
                break;
            case 38:
                // Truecolor Foreground (xterm-256color)
                if (params.at(i + 1).value_or(0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                set_fg(Color { (uint8_t) di::clamp(params[i + 2], 0, 255), (uint8_t) di::clamp(params[i + 3], 0, 255),
                               (uint8_t) di::clamp(params[i + 4], 0, 255) });
                i += 4;
                break;
            case 39:
                reset_fg();
                break;
            case 40:
                set_bg({ Color::Palette::Black });
                break;
            case 41:
                set_bg({ Color::Palette::Red });
                break;
            case 42:
                set_bg({ Color::Palette::Green });
                break;
            case 43:
                set_bg({ Color::Palette::Brown });
                break;
            case 44:
                set_bg({ Color::Palette::Blue });
                break;
            case 45:
                set_bg({ Color::Palette::Magenta });
                break;
            case 46:
                set_bg({ Color::Palette::Cyan });
                break;
            case 47:
                set_bg({ Color::Palette::LightGrey });
                break;
            case 48:
                // Truecolor Background (xterm-256color)
                if (params.at(i + 1).value_or(0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                set_bg(Color { (uint8_t) di::clamp(params[i + 2], 0, 255), (uint8_t) di::clamp(params[i + 3], 0, 255),
                               (uint8_t) di::clamp(params[i + 4], 0, 255) });
                i += 4;
                break;
            case 49:
                reset_bg();
                break;
            case 90:
                set_fg({ Color::Palette::DarkGrey });
                break;
            case 91:
                set_fg({ Color::Palette::LightRed });
                break;
            case 92:
                set_fg({ Color::Palette::LightGreen });
                break;
            case 93:
                set_fg({ Color::Palette::Yellow });
                break;
            case 94:
                set_fg({ Color::Palette::LightBlue });
                break;
            case 95:
                set_fg({ Color::Palette::LightMagenta });
                break;
            case 96:
                set_fg({ Color::Palette::LightCyan });
                break;
            case 97:
                set_fg({ Color::Palette::White });
                break;
            case 100:
                set_bg({ Color::Palette::DarkGrey });
                break;
            case 101:
                set_bg({ Color::Palette::LightRed });
                break;
            case 102:
                set_bg({ Color::Palette::LightGreen });
                break;
            case 103:
                set_bg({ Color::Palette::Yellow });
                break;
            case 104:
                set_bg({ Color::Palette::LightBlue });
                break;
            case 105:
                set_bg({ Color::Palette::LightMagenta });
                break;
            case 106:
                set_bg({ Color::Palette::LightCyan });
                break;
            case 107:
                set_bg({ Color::Palette::White });
                break;
            default:
                break;
        }
    }
}

// Device Status Report - https://vt100.net/docs/vt510-rm/DSR.html
void Terminal::csi_dsr(di::Vector<int> const& params) {
    switch (params.at(0).value_or(0)) {
        case 5:
            // Operating Status - https://vt100.net/docs/vt510-rm/DSR-OS.html
            (void) m_psuedo_terminal.write_exactly(di::as_bytes("\033[0n"_sv.span()));
            break;
        case 6:
            // Cursor Position Report - https://vt100.net/docs/vt510-rm/DSR-CPR.html
            (void) m_psuedo_terminal.write_exactly(
                di::as_bytes(di::present("\033[{};{}R"_sv, m_cursor_row + 1, m_cursor_col + 1).value().span()));
            break;
        default:
            break;
    }
}

// DEC Set Top and Bottom Margins - https://www.vt100.net/docs/vt100-ug/chapter3.html#DECSTBM
void Terminal::csi_decstbm(di::Vector<int> const& params) {
    int new_scroll_start = params.at(0).value_or(1) - 1;
    int new_scroll_end = params.at(1).value_or(m_row_count) - 1;
    if (new_scroll_end - new_scroll_start < 2) {
        return;
    }
    m_rows_above.clear();
    m_rows_below.clear();
    m_scroll_start = new_scroll_start;
    m_scroll_end = new_scroll_end;
    set_cursor(0, 0);
}

// Save Current Cursor Position - https://vt100.net/docs/vt510-rm/SCOSC.html
void Terminal::csi_scosc(di::Vector<int> const&) {
    save_pos();
}

// Restore Saved Cursor Position - https://vt100.net/docs/vt510-rm/SCORC.html
void Terminal::csi_scorc(di::Vector<int> const&) {
    restore_pos();
}

void Terminal::set_cursor(int row, int col) {
    m_cursor_row = di::clamp(row, min_row_inclusive(), max_row_inclusive());
    m_cursor_col = di::clamp(col, min_col_inclusive(), max_col_inclusive());
    m_x_overflow = false;
}

void Terminal::set_visible_size(int rows, int cols) {
    m_available_rows_in_display = rows;
    m_available_cols_in_display = cols;
    if (!m_80_col_mode && !m_132_col_mode) {
        resize(rows, cols);
    }
}

void Terminal::resize(int rows, int cols) {
    m_row_count = rows;
    m_col_count = cols;

    m_scroll_start = 0;
    m_scroll_end = rows - 1;

    m_rows.resize(rows);
    for (auto& row : m_rows) {
        row.resize(cols);
    }

    for (auto& row : m_rows_above) {
        row.resize(cols);
    }

    for (auto& row : m_rows_below) {
        row.resize(cols);
    }

    set_cursor(m_cursor_row, m_cursor_col);

    invalidate_all();
}

void Terminal::invalidate_all() {
    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }
}

void Terminal::clear_below_cursor(char ch) {
    clear_row_to_end(m_cursor_row, m_cursor_col, ch);
    for (auto r = m_cursor_row + 1; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void Terminal::clear_above_cursor(char ch) {
    for (auto r = 0; r < m_cursor_row; r++) {
        clear_row(r, ch);
    }
    clear_row_until(m_cursor_row, m_cursor_col, ch);
}

void Terminal::clear(char ch) {
    for (auto r = 0; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void Terminal::clear_row(int r, char ch) {
    clear_row_to_end(r, 0, ch);
}

void Terminal::clear_row_until(int r, int end_col, char ch) {
    for (auto c = 0; c <= end_col; c++) {
        put_char(r, c, ch);
    }
}

void Terminal::clear_row_to_end(int r, int start_col, char ch) {
    for (auto c = start_col; c < m_col_count; c++) {
        put_char(r, c, ch);
    }
}

void Terminal::put_char(int row, int col, c32 c) {
    auto& cell = m_rows[row][col];
    cell.ch = c;
    cell.bg = m_bg;
    cell.fg = m_fg;
    cell.bold = m_bold;
    cell.dim = m_dim;
    cell.italic = m_italic;
    cell.underline = m_underline;
    cell.blink = m_blink;
    cell.rapid_blink = m_rapid_blink;
    cell.inverted = m_inverted;
    cell.invisible = m_invisible;
    cell.strike_through = m_strike_through;
    cell.dirty = true;
}

void Terminal::put_char(c32 c) {
    if ((c >= 0 && c <= 31) || c == 127) {
        put_char('^');
        put_char(c | 0x40);
        return;
    }

    if (m_x_overflow) {
        m_cursor_row++;
        scroll_down_if_needed();
        m_cursor_col = 0;
        m_x_overflow = false;
    }

    put_char(m_cursor_row, m_cursor_col, c);

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_x_overflow = m_autowrap_mode;
        m_cursor_col--;
    }
}

bool Terminal::should_display_cursor_at_position(int r, int c) const {
    if (m_cursor_hidden) {
        return false;
    }

    if (c != m_cursor_col) {
        return false;
    }

    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end || r < m_scroll_start || r > m_scroll_end) {
        return r == m_cursor_row;
    }

    return row_offset() + r == cursor_row() + total_rows() - row_count();
}

int Terminal::scroll_relative_offset(int display_row) const {
    if (display_row < m_scroll_start) {
        return display_row;
    } else if (display_row > m_scroll_end) {
        return display_row + total_rows() - row_count();
    }
    return display_row + row_offset();
}

Terminal::Row const& Terminal::row_at_scroll_relative_offset(int offset) const {
    if (offset < m_scroll_start) {
        return m_rows[offset];
    }
    if (offset < m_scroll_start + (int) m_rows_above.size()) {
        return m_rows_above[offset - m_scroll_start];
    }
    if (offset < m_scroll_start + (int) m_rows_above.size() + (m_scroll_end - m_scroll_start)) {
        return m_rows[offset - m_rows_above.size()];
    }
    if (offset <
        m_scroll_start + (int) m_rows_above.size() + (m_scroll_end - m_scroll_start) + (int) m_rows_below.size()) {
        return m_rows_below[offset - m_scroll_start - m_rows_above.size() - (m_scroll_end - m_scroll_start)];
    }
    return m_rows[offset - m_rows_above.size() - m_rows_below.size()];
}

void Terminal::set_use_alternate_screen_buffer(bool b) {
    if ((!b && !m_save_state) || (b && m_save_state)) {
        return;
    }

    if (b) {
        m_save_state = di::make_box<Terminal>(di::clone(*this));
        reset_attributes();
        m_x_overflow = false;
        m_cursor_hidden = false;
        m_cursor_row = m_cursor_col = m_saved_cursor_row = m_saved_cursor_col = 0;
        m_rows.resize(m_row_count);
        m_rows_above.clear();
        m_rows_below.clear();
        clear();
    } else {
        ASSERT(m_save_state);
        m_cursor_row = m_save_state->m_cursor_row;
        m_cursor_col = m_save_state->m_cursor_col;
        m_saved_cursor_row = m_save_state->m_saved_cursor_row;
        m_saved_cursor_col = m_save_state->m_saved_cursor_col;
        m_bold = m_save_state->m_bold;
        m_inverted = m_save_state->m_inverted;
        m_bg = m_save_state->m_bg;
        m_fg = m_save_state->m_fg;
        m_x_overflow = m_save_state->m_x_overflow;
        m_cursor_hidden = m_save_state->m_cursor_hidden;
        m_rows = di::move(m_save_state->m_rows);
        m_rows_above = di::move(m_save_state->m_rows_above);
        m_rows_below = di::move(m_save_state->m_rows_below);

        if (m_row_count != m_save_state->m_row_count || m_col_count != m_save_state->m_col_count) {
            resize(m_row_count, m_col_count);
        } else {
            invalidate_all();
        }

        m_save_state = nullptr;
    }
}

void Terminal::scroll_up() {
    if (m_rows_above.empty()) {
        return;
    }

    di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
    // m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
    m_rows_below.push_back(di::move(m_rows[m_scroll_start]));
    m_rows[m_scroll_start] = m_rows_above.pop_back().value();
    invalidate_all();
}

void Terminal::scroll_down() {
    if (m_rows_below.empty()) {
        return;
    }

    di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_cursor_row + 1, m_rows.begin() + m_scroll_end + 1);
    // m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
    m_rows_above.push_back(di::move(m_rows[m_scroll_end]));
    m_rows[m_scroll_end] = m_rows_below.pop_back().value();
    invalidate_all();
}

void Terminal::scroll_up_if_needed() {
    if (m_cursor_row == m_scroll_start - 1) {
        m_cursor_row = di::clamp(m_cursor_row, m_scroll_start, m_scroll_end);

        if (!m_rows_above.empty()) {
            scroll_up();
            return;
        }

        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_scroll_end, m_rows.begin() + m_scroll_end + 1);
        // m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
        m_rows_below.push_back(di::move(m_rows[m_scroll_start]));
        m_rows[m_scroll_start] = Row();
        m_rows[m_scroll_start].resize(m_col_count);
        invalidate_all();

        if (total_rows() - (int) m_rows.size() > m_row_count + 100) {
            m_rows_below.erase(m_rows_below.begin());
        }
    }
}

void Terminal::scroll_down_if_needed() {
    if (m_cursor_row == m_scroll_end + 1) {
        m_cursor_row = di::clamp(m_cursor_row, m_scroll_start, m_scroll_end);

        if (!m_rows_below.empty()) {
            scroll_down();
            return;
        }

        di::rotate(m_rows.begin() + m_cursor_row, m_rows.begin() + m_cursor_row + 1, m_rows.begin() + m_scroll_end + 1);
        // m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
        m_rows_above.push_back(di::move(m_rows[m_scroll_end]));
        m_rows[m_scroll_end] = Row();
        m_rows[m_scroll_end].resize(m_col_count);
        invalidate_all();

        if (total_rows() - (int) m_rows.size() > m_row_count + 100) {
            m_rows_above.erase(m_rows_above.begin());
        }
    }
}

void Terminal::scroll_to_bottom() {
    while (!m_rows_below.empty()) {
        scroll_down();
    }
}
}
