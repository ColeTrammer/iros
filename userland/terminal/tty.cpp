#include <ctype.h>
#include <errno.h>
#include <liim/byte_io.h>
#include <stdlib.h>

#include "pseudo_terminal.h"
#include "tty.h"

// #define TERMINAL_DEBUG

void TTY::on_printable_character(uint8_t byte) {
    if (byte < 0x7F) {
        put_char(byte);
    }
}

void TTY::on_c0_character(uint8_t byte) {
    switch (byte) {
        case 8:
            return c0_bs();
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

    fprintf(stderr, "Unknown C0: '%#X' '%#o' '%d'\n", byte, byte, byte);
}

void TTY::on_csi(const String& intermediate, const Vector<int>& params, uint8_t byte) {
    if (intermediate == "=") {
        switch (byte) {
            case 'c':
                return csi_da3(params);
        }

        fprintf(stderr, "Unknown escape: CSI %s %c\n", intermediate.string(), byte);
        return;
    }

    if (intermediate == ">") {
        switch (byte) {
            case 'c':
                return csi_da2(params);
        }

        fprintf(stderr, "Unknown escape: CSI %s %c\n", intermediate.string(), byte);
        return;
    }

    if (intermediate == "?") {
        switch (byte) {
            case 'h':
                return csi_decset(params);
            case 'l':
                return csi_decrst(params);
        }

        fprintf(stderr, "Unknown escape: CSI %s %c\n", intermediate.string(), byte);
        return;
    }

    if (intermediate != "") {
        fprintf(stderr, "Unknown escape: CSI %s %c\n", intermediate.string(), byte);
        return;
    }

    switch (byte) {
        case '@':
            return csi_ich(params);
        case 'A':
            return csi_cuu(params);
        case 'B':
            return csi_cud(params);
        case 'C':
            return csi_cuf(params);
        case 'D':
            return csi_cub(params);
        case 'G':
            return csi_cha(params);
        case 'H':
            return csi_cup(params);
        case 'J':
            return csi_ed(params);
        case 'K':
            return csi_el(params);
        case 'L':
            return csi_il(params);
        case 'M':
            return csi_dl(params);
        case 'P':
            return csi_dch(params);
        case 'S':
            return csi_su(params);
        case 'T':
            return csi_sd(params);
        case 'X':
            return csi_ech(params);
        case 'b':
            return csi_rep(params);
        case 'c':
            return csi_da1(params);
        case 'd':
            return csi_vpa(params);
        case 'f':
            return csi_hvp(params);
        case 'g':
            return csi_tbc(params);
        case 'm':
            return csi_sgr(params);
        case 'n':
            return csi_dsr(params);
        case 'r':
            return csi_decstbm(params);
        case 's':
            return csi_scosc(params);
        case 'u':
            return csi_scorc(params);
    }

    fprintf(stderr, "Unknown escape: CSI %s %c\n", intermediate.string(), byte);
    return;
}

void TTY::on_escape(const String& intermediate, uint8_t byte) {
    if (intermediate == "#") {
        switch (byte) {
            case '8':
                return esc_decaln();
        }
        fprintf(stderr, "Unknown escape: ESC %s %c\n", intermediate.string(), byte);
        return;
    }

    if (intermediate != "") {
        fprintf(stderr, "Unknown escape: ESC %s %c\n", intermediate.string(), byte);
        return;
    }

    switch (byte) {
        case '7':
            return esc_decsc();
        case '8':
            return esc_decrc();
    }

    // 8 bit control characters
    switch (byte) {
        case 'D':
            return c1_ind();
        case 'E':
            return c1_nel();
        case 'H':
            return c1_hts();
        case 'M':
            return c1_ri();
    }

    fprintf(stderr, "Unknown escape: ESC %s %c\n", intermediate.string(), byte);
}

// Backspace - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_bs() {
    if (m_cursor_col) {
        m_cursor_col--;
    }
    m_x_overflow = false;
}

// Horizontal Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_ht() {
    for (auto tab_stop : m_tab_stops) {
        if (tab_stop > m_cursor_col) {
            return set_cursor(m_cursor_row, tab_stop);
        }
    }

    set_cursor(m_cursor_row, max_col_inclusive());
}

// Line Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_lf() {
    m_cursor_row++;
    scroll_down_if_needed();
    m_x_overflow = false;
}

// Vertical Tab - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_vt() {
    c0_lf();
}

// Form Feed - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_ff() {
    c0_lf();
}

// Carriage Return - https://vt100.net/docs/vt510-rm/chapter4.html#T4-1
void TTY::c0_cr() {
    m_cursor_col = 0;
    m_x_overflow = false;
}

// Index - https://vt100.net/docs/vt510-rm/IND.html
void TTY::c1_ind() {
    m_cursor_row++;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Next Line - https://vt100.net/docs/vt510-rm/NEL.html
void TTY::c1_nel() {
    m_cursor_row++;
    m_cursor_col = 0;
    m_x_overflow = false;
    scroll_down_if_needed();
}

// Horizontal Tab Set - https://vt100.net/docs/vt510-rm/HTS.html
void TTY::c1_hts() {
    if (m_tab_stops.includes(m_cursor_col)) {
        return;
    }

    int index = 0;
    for (; index < m_tab_stops.size(); index++) {
        if (m_cursor_col < m_tab_stops[index]) {
            break;
        }
    }

    m_tab_stops.insert(m_cursor_col, index);
}

// Reverse Index - https://www.vt100.net/docs/vt100-ug/chapter3.html#RI
void TTY::c1_ri() {
    m_cursor_row--;
    m_x_overflow = false;
    scroll_up_if_needed();
}

// DEC Screen Alignment Pattern - https://vt100.net/docs/vt510-rm/DECALN.html
void TTY::esc_decaln() {
    clear('E');
    set_cursor(0, 0);
    m_x_overflow = false;
}

// DEC Save Cursor - https://vt100.net/docs/vt510-rm/DECSC.html
void TTY::esc_decsc() {
    save_pos();
}

// DEC Restore Cursor - https://vt100.net/docs/vt510-rm/DECRC.html
void TTY::esc_decrc() {
    restore_pos();
}

// Insert Character - https://vt100.net/docs/vt510-rm/ICH.html
void TTY::csi_ich(const Vector<int>& params) {
    auto chars = max(1, params.get_or(0, 1));

    auto& row = m_rows[m_cursor_row];
    for (int i = max_col_inclusive() - chars; i >= m_cursor_col; i--) {
        row[i + chars] = row[i];
    }

    for (int i = m_cursor_col; i <= max_col_inclusive() && i < m_cursor_col + chars; i++) {
        row[i] = {};
    }
}

// Cursor Up - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUU
void TTY::csi_cuu(const Vector<int>& params) {
    auto delta_row = max(1, params.get_or(0, 1));
    set_cursor(m_cursor_row - delta_row, m_cursor_col);
}

// Cursor Down - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUD
void TTY::csi_cud(const Vector<int>& params) {
    auto delta_row = max(1, params.get_or(0, 1));
    set_cursor(m_cursor_row + delta_row, m_cursor_col);
}

// Cursor Forward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUF
void TTY::csi_cuf(const Vector<int>& params) {
    auto delta_col = max(1, params.get_or(0, 1));
    set_cursor(m_cursor_row, m_cursor_col + delta_col);
}

// Cursor Backward - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUB
void TTY::csi_cub(const Vector<int>& params) {
    auto delta_col = max(1, params.get_or(0, 1));
    set_cursor(m_cursor_row, m_cursor_col - delta_col);
}

// Cursor Position - https://www.vt100.net/docs/vt100-ug/chapter3.html#CUP
void TTY::csi_cup(const Vector<int>& params) {
    auto row = translate_row(params.get_or(0, 1));
    auto col = translate_col(params.get_or(1, 1));
    set_cursor(row, col);
}

// Cursor Horizontal Absolute - https://vt100.net/docs/vt510-rm/CHA.html
void TTY::csi_cha(const Vector<int>& params) {
    set_cursor(m_cursor_row, translate_col(params.get_or(0, 1)));
}

// Erase in Display - https://vt100.net/docs/vt510-rm/ED.html
void TTY::csi_ed(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
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
void TTY::csi_el(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
        case 0:
            return clear_row_to_end(m_cursor_row, m_cursor_col);
        case 1:
            return clear_row_until(m_cursor_row, m_cursor_col);
        case 2:
            return clear_row(m_cursor_row);
    }
}

// Insert Line - https://vt100.net/docs/vt510-rm/IL.html
void TTY::csi_il(const Vector<int>& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    int lines_to_insert = max(1, params.get_or(0, 1));
    for (int i = 0; i < lines_to_insert; i++) {
        m_rows.rotate_right(m_cursor_row, m_scroll_end + 1);
        m_rows[m_cursor_row] = Row(m_col_count);
        m_rows[m_cursor_row].resize(m_col_count);
    }
    invalidate_all();
}

// Delete Line - https://vt100.net/docs/vt510-rm/DL.html
void TTY::csi_dl(const Vector<int>& params) {
    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
        return;
    }
    int lines_to_delete = clamp(params.get_or(0, 1), 1, m_scroll_end - m_cursor_row);
    for (int i = 0; i < lines_to_delete; i++) {
        m_rows.rotate_left(m_cursor_row, m_scroll_end + 1);
        m_rows[m_scroll_end] = Row(m_col_count);
        m_rows[m_scroll_end].resize(m_col_count);
    }

    invalidate_all();
}

// Delete Character - https://vt100.net/docs/vt510-rm/DCH.html
void TTY::csi_dch(const Vector<int>& params) {
    int chars_to_delete = clamp(params.get_or(0, 1), 1, m_col_count - m_cursor_col);
    for (int i = 0; i < chars_to_delete; i++) {
        m_rows[m_cursor_row].remove(m_cursor_col);
    }
    m_rows[m_cursor_row].resize(m_col_count);
    for (int i = m_cursor_col; i < m_col_count; i++) {
        m_rows[m_cursor_row][i].dirty = true;
    }
}

// Pan Down - https://vt100.net/docs/vt510-rm/SU.html
void TTY::csi_su(const Vector<int>& params) {
    int to_scroll = params.get_or(0, 1);
    int row_save = m_cursor_row;
    for (int i = 0; i < to_scroll; i++) {
        m_cursor_row = m_row_count;
        scroll_down_if_needed();
    }
    m_cursor_row = row_save;
    return;
}

// Pan Up - https://vt100.net/docs/vt510-rm/SD.html
void TTY::csi_sd(const Vector<int>& params) {
    int to_scroll = params.get_or(0, 1);
    int row_save = m_cursor_row;
    for (int i = 0; i < to_scroll; i++) {
        m_cursor_row = -1;
        scroll_up_if_needed();
    }
    m_cursor_row = row_save;
}

// Erase Character - https://vt100.net/docs/vt510-rm/ECH.html
void TTY::csi_ech(const Vector<int>& params) {
    int chars_to_erase = max(1, params.get_or(0, 1));
    for (int i = m_cursor_col; i - m_cursor_col < chars_to_erase && i < m_col_count; i++) {
        m_rows[m_cursor_row][i] = {};
    }
}

// Repeat Preceding Graphic Character - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void TTY::csi_rep(const Vector<int>& params) {
    char preceding_character = ' ';
    if (m_cursor_col == 0) {
        if (m_cursor_row != 0) {
            preceding_character = m_rows[m_cursor_row - 1][m_col_count - 1].ch;
        }
    } else {
        preceding_character = m_rows[m_cursor_row][m_cursor_col - 1].ch;
    }
    for (int i = 0; i < params.get_or(0, 0); i++) {
        put_char(preceding_character);
    }
}

// Primary Device Attributes - https://vt100.net/docs/vt510-rm/DA1.html
void TTY::csi_da1(const Vector<int>& params) {
    if (params.get_or(0, 0) != 0) {
        return;
    }
    m_psuedo_terminal.write("\033[?1;0c");
}

// Secondary Device Attributes - https://vt100.net/docs/vt510-rm/DA2.html
void TTY::csi_da2(const Vector<int>& params) {
    if (params.get_or(0, 0) != 0) {
        return;
    }
    m_psuedo_terminal.write("\033[>010;0c");
}

// Tertiary Device Attributes - https://vt100.net/docs/vt510-rm/DA3.html
void TTY::csi_da3(const Vector<int>& params) {
    if (params.get_or(0, 0) != 0) {
        return;
    }
    m_psuedo_terminal.write("\033P!|00000000\033\\");
}

// Vertical Line Position Absolute - https://vt100.net/docs/vt510-rm/VPA.html
void TTY::csi_vpa(const Vector<int>& params) {
    set_cursor(translate_row(params.get_or(0, 1)), m_cursor_col);
}

// Horizontal and Vertical Position - https://vt100.net/docs/vt510-rm/HVP.html
void TTY::csi_hvp(const Vector<int>& params) {
    csi_cup(params);
}

// Tab Clear - https://vt100.net/docs/vt510-rm/TBC.html
void TTY::csi_tbc(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
        case 0:
            m_tab_stops.remove_if([this](auto x) {
                return x == m_cursor_col;
            });
            return;
        case 3:
            m_tab_stops.clear();
            return;
    }
}

// DEC Private Mode Set - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void TTY::csi_decset(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            m_psuedo_terminal.set_application_cursor_keys(true);
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
            m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
            m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
            break;
        case 25:
            // Text Cursor Enable Mode - https://vt100.net/docs/vt510-rm/DECTCEM.html
            m_cursor_hidden = false;
            break;
        case 40:
            m_allow_80_132_col_mode = true;
            break;
        case 1000:
            m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
            m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
            break;
        case 1001:
            m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X11);
            m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1002:
            m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::Cell);
            m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1003:
            m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::All);
            m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            break;
        case 1005:
            if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::UTF8);
            }
            break;
        case 1006:
            if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::SGR);
            }
            break;
        case 1015:
            if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::URXVT);
            }
            break;
        case 1049:
            set_use_alternate_screen_buffer(true);
            break;
        case 2004:
            m_psuedo_terminal.set_bracketed_paste(true);
            break;
        default:
            fprintf(stderr, "Unsupported DEC Set %d\n", params.get_or(0, 0));
            break;
    }
}

// DEC Private Mode Reset - https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
void TTY::csi_decrst(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
        case 1:
            // Cursor Keys Mode - https://vt100.net/docs/vt510-rm/DECCKM.html
            m_psuedo_terminal.set_application_cursor_keys(false);
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
            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X10);
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
            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X11);
            break;
        case 1001:
            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Hilite);
            break;
        case 1002:
            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Cell);
            break;
        case 1003:
            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::All);
            break;
        case 1005:
            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::UTF8) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            }
            break;
        case 1006:
            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::SGR) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            }
            break;
        case 1015:
            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::URXVT) {
                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
            }
            break;
        case 1049:
            set_use_alternate_screen_buffer(false);
            break;
        case 2004:
            m_psuedo_terminal.set_bracketed_paste(false);
            break;
        default:
            fprintf(stderr, "Unsupported DEC Reset %d\n", params.get_or(0, 0));
            break;
    }
}

// Select Graphics Rendition - https://vt100.net/docs/vt510-rm/SGR.html
void TTY::csi_sgr(const Vector<int>& params) {
    for (int i = 0; i == 0 || i < params.size(); i++) {
        switch (params.get_or(i, 0)) {
            case 0:
                reset_attributes();
                break;
            case 1:
                set_bold(true);
                break;
            case 7:
                set_inverted(true);
                break;
            case 30:
                set_fg(VGA_COLOR_BLACK);
                break;
            case 31:
                set_fg(VGA_COLOR_RED);
                break;
            case 32:
                set_fg(VGA_COLOR_GREEN);
                break;
            case 33:
                set_fg(VGA_COLOR_BROWN);
                break;
            case 34:
                set_fg(VGA_COLOR_BLUE);
                break;
            case 35:
                set_fg(VGA_COLOR_MAGENTA);
                break;
            case 36:
                set_fg(VGA_COLOR_CYAN);
                break;
            case 37:
                set_fg(VGA_COLOR_LIGHT_GREY);
                break;
            case 38:
                // Truecolor Foreground (xterm-256color)
                if (params.get_or(i + 1, 0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                set_fg({ (uint8_t) clamp(params[i + 2], 0, 255), (uint8_t) clamp(params[i + 3], 0, 255),
                         (uint8_t) clamp(params[i + 4], 0, 255) });
                i += 4;
                break;
            case 39:
                reset_fg();
                break;
            case 40:
                set_bg(VGA_COLOR_BLACK);
                break;
            case 41:
                set_bg(VGA_COLOR_RED);
                break;
            case 42:
                set_bg(VGA_COLOR_GREEN);
                break;
            case 43:
                set_bg(VGA_COLOR_BROWN);
                break;
            case 44:
                set_bg(VGA_COLOR_BLUE);
                break;
            case 45:
                set_bg(VGA_COLOR_MAGENTA);
                break;
            case 46:
                set_bg(VGA_COLOR_CYAN);
                break;
            case 47:
                set_bg(VGA_COLOR_LIGHT_GREY);
                break;
            case 48:
                // Truecolor Foreground (xterm-256color)
                if (params.get_or(i + 1, 0) != 2) {
                    break;
                }
                if (params.size() - i < 5) {
                    break;
                }
                set_bg({ (uint8_t) clamp(params[i + 2], 0, 255), (uint8_t) clamp(params[i + 3], 0, 255),
                         (uint8_t) clamp(params[i + 4], 0, 255) });
                i += 4;
                break;
            case 49:
                reset_bg();
                break;
            case 90:
                set_fg(VGA_COLOR_DARK_GREY);
                break;
            case 91:
                set_fg(VGA_COLOR_LIGHT_RED);
                break;
            case 92:
                set_fg(VGA_COLOR_LIGHT_GREEN);
                break;
            case 93:
                set_fg(VGA_COLOR_YELLOW);
                break;
            case 94:
                set_fg(VGA_COLOR_LIGHT_BLUE);
                break;
            case 95:
                set_fg(VGA_COLOR_LIGHT_MAGENTA);
                break;
            case 96:
                set_fg(VGA_COLOR_LIGHT_CYAN);
                break;
            case 97:
                set_fg(VGA_COLOR_WHITE);
                break;
            case 100:
                set_bg(VGA_COLOR_DARK_GREY);
                break;
            case 101:
                set_bg(VGA_COLOR_LIGHT_RED);
                break;
            case 102:
                set_bg(VGA_COLOR_LIGHT_GREEN);
                break;
            case 103:
                set_bg(VGA_COLOR_YELLOW);
                break;
            case 104:
                set_bg(VGA_COLOR_LIGHT_BLUE);
                break;
            case 105:
                set_bg(VGA_COLOR_LIGHT_MAGENTA);
                break;
            case 106:
                set_bg(VGA_COLOR_LIGHT_CYAN);
                break;
            case 107:
                set_bg(VGA_COLOR_WHITE);
                break;
            default:
                break;
        }
    }
}

// Device Status Report - https://vt100.net/docs/vt510-rm/DSR.html
void TTY::csi_dsr(const Vector<int>& params) {
    switch (params.get_or(0, 0)) {
        case 5:
            // Operating Status - https://vt100.net/docs/vt510-rm/DSR-OS.html
            m_psuedo_terminal.write("\033[0n");
            break;
        case 6:
            // Cursor Position Report - https://vt100.net/docs/vt510-rm/DSR-CPR.html
            m_psuedo_terminal.write(String::format("\033[%d;%dR", m_cursor_row + 1, m_cursor_col + 1));
            break;
        default:
            break;
    }
}

// DEC Set Top and Bottom Margins - https://www.vt100.net/docs/vt100-ug/chapter3.html#DECSTBM
void TTY::csi_decstbm(const Vector<int>& params) {
    int new_scroll_start = params.get_or(0, 1) - 1;
    int new_scroll_end = params.get_or(1, m_row_count) - 1;
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
void TTY::csi_scosc(const Vector<int>&) {
    save_pos();
}

// Restore Saved Cursor Position - https://vt100.net/docs/vt510-rm/SCORC.html
void TTY::csi_scorc(const Vector<int>&) {
    restore_pos();
}

void TTY::set_cursor(int row, int col) {
    m_cursor_row = clamp(row, min_row_inclusive(), max_row_inclusive());
    m_cursor_col = clamp(col, min_col_inclusive(), max_col_inclusive());
    m_x_overflow = false;
}

void TTY::set_visible_size(int rows, int cols) {
    m_available_rows_in_display = rows;
    m_available_cols_in_display = cols;
    if (!m_80_col_mode && !m_132_col_mode) {
        resize(rows, cols);
    }
}

void TTY::resize(int rows, int cols) {
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

void TTY::invalidate_all() {
    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }
}

void TTY::clear_below_cursor(char ch) {
    clear_row_to_end(m_cursor_row, m_cursor_col, ch);
    for (auto r = m_cursor_row + 1; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void TTY::clear_above_cursor(char ch) {
    for (auto r = 0; r < m_cursor_row; r++) {
        clear_row(r, ch);
    }
    clear_row_until(m_cursor_row, m_cursor_col, ch);
}

void TTY::clear(char ch) {
    for (auto r = 0; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void TTY::clear_row(int r, char ch) {
    clear_row_to_end(r, 0, ch);
}

void TTY::clear_row_until(int r, int end_col, char ch) {
    for (auto c = 0; c <= end_col; c++) {
        put_char(r, c, ch);
    }
}

void TTY::clear_row_to_end(int r, int start_col, char ch) {
    for (auto c = start_col; c < m_col_count; c++) {
        put_char(r, c, ch);
    }
}

void TTY::put_char(int row, int col, char c) {
    auto& cell = m_rows[row][col];
    cell.ch = c;
    cell.bg = m_bg;
    cell.fg = m_fg;
    cell.bold = m_bold;
    cell.inverted = m_inverted;
    cell.dirty = true;
}

void TTY::put_char(char c) {
    if (iscntrl(c)) {
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

bool TTY::should_display_cursor_at_position(int r, int c) const {
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

int TTY::scroll_relative_offset(int display_row) const {
    if (display_row < m_scroll_start) {
        return display_row;
    } else if (display_row > m_scroll_end) {
        return display_row + total_rows() - row_count();
    }
    return display_row + row_offset();
}

const TTY::Row& TTY::row_at_scroll_relative_offset(int offset) const {
    if (offset < m_scroll_start) {
        return m_rows[offset];
    }
    if (offset < m_scroll_start + m_rows_above.size()) {
        return m_rows_above[offset - m_scroll_start];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start)) {
        return m_rows[offset - m_rows_above.size()];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start) + m_rows_below.size()) {
        return m_rows_below[offset - m_scroll_start - m_rows_above.size() - (m_scroll_end - m_scroll_start)];
    }
    return m_rows[offset - m_rows_above.size() - m_rows_below.size()];
}

void TTY::set_use_alternate_screen_buffer(bool b) {
    if ((!b && !m_save_state) || (b && m_save_state)) {
        return;
    }

    if (b) {
        m_save_state = make_shared<TTY>(*this);
        reset_attributes();
        m_x_overflow = false;
        m_cursor_hidden = false;
        m_cursor_row = m_cursor_col = m_saved_cursor_row = m_saved_cursor_col = 0;
        m_rows.resize(m_row_count);
        m_rows_above.clear();
        m_rows_below.clear();
        clear();
    } else {
        assert(m_save_state);
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
        m_rows = move(m_save_state->m_rows);
        m_rows_above = move(m_save_state->m_rows_above);
        m_rows_below = move(m_save_state->m_rows_below);

        if (m_row_count != m_save_state->m_row_count || m_col_count != m_save_state->m_col_count) {
            resize(m_row_count, m_col_count);
        } else {
            invalidate_all();
        }

        m_save_state = nullptr;
    }
}

void TTY::scroll_up() {
    if (m_rows_above.empty()) {
        return;
    }

    m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
    m_rows_below.add(move(m_rows[m_scroll_start]));
    m_rows[m_scroll_start] = move(m_rows_above.last());
    m_rows_above.remove_last();
    invalidate_all();
}

void TTY::scroll_down() {
    if (m_rows_below.empty()) {
        return;
    }

    m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
    m_rows_above.add(move(m_rows[m_scroll_end]));
    m_rows[m_scroll_end] = move(m_rows_below.last());
    m_rows_below.remove_last();
    invalidate_all();
}

void TTY::scroll_up_if_needed() {
    if (m_cursor_row == m_scroll_start - 1) {
        m_cursor_row = clamp(m_cursor_row, m_scroll_start, m_scroll_end);

        if (!m_rows_above.empty()) {
            scroll_up();
            return;
        }

        m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
        m_rows_below.add(move(m_rows[m_scroll_start]));
        m_rows[m_scroll_start] = Row(m_col_count);
        m_rows[m_scroll_start].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 100) {
            m_rows_below.remove(0);
        }
    }
}

void TTY::scroll_down_if_needed() {
    if (m_cursor_row == m_scroll_end + 1) {
        m_cursor_row = clamp(m_cursor_row, m_scroll_start, m_scroll_end);

        if (!m_rows_below.empty()) {
            scroll_down();
            return;
        }

        m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
        m_rows_above.add(move(m_rows[m_scroll_end]));
        m_rows[m_scroll_end] = Row(m_col_count);
        m_rows[m_scroll_end].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 100) {
            m_rows_above.remove(0);
        }
    }
}

void TTY::scroll_to_bottom() {
    while (!m_rows_below.empty()) {
        scroll_down();
    }
}

void TTY::on_input(Span<const uint8_t> input) {
    m_parser->parse(input);
}
