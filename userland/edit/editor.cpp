#include <errno.h>
#include <stdio.h>

#include "editor.h"
#include "panel.h"

#ifndef display_error
#define display_error(format, ...)                          \
    do {                                                    \
        fprintf(stderr, format __VA_OPT__(, ) __VA_ARGS__); \
        fprintf(stderr, ": %s\n", strerror(errno));         \
    } while (0)
#endif /* display_error */

UniquePtr<Document> Document::create_from_file(const String& path, Panel& panel) {
    FILE* file = fopen(path.string(), "r");
    if (!file) {
        display_error("edit: error reading file: `%s'", path.string());
        return nullptr;
    }

    Vector<Line> lines;
    char* line = nullptr;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, file)) != -1) {
        char* trailing_newline = strchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        lines.add(Line(String(line)));
    }

    UniquePtr<Document> ret;

    if (ferror(file)) {
        display_error("edit: error reading file: `%s'", path.string());
    } else {
        ret = make_unique<Document>(move(lines), path, panel);
    }

    if (fclose(file)) {
        display_error("edit: error closing file: `%s'", path.string());
    }

    return ret;
}

void Document::render_line(int line_number, int row_in_panel) const {
    auto& line = m_lines[line_number];
    for (int i = 0; i < line.contents().size() && i < m_panel.cols(); i++) {
        m_panel.set_text_at(row_in_panel, i, line.contents()[i]);
    }
}

void Document::display() const {
    m_panel.clear();
    for (int line_num = 0; line_num < m_lines.size() && line_num < m_panel.rows(); line_num++) {
        render_line(line_num, line_num);
    }
    m_panel.flush();
}