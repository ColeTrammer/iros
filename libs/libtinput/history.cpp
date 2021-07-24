#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinput/history.h>

namespace TInput {

History::History(String path, int history_max) : m_history(history_max), m_history_max(history_max), m_path(move(path)) {
    read_history();
}

History::~History() {
    if (should_write_history()) {
        write_history();
    }
}

void History::print_history() {
    for (int i = 0; i < size(); i++) {
        printf("%*d %s\n", 4, i + 1, item(i).string());
    }
}

void History::read_history() {
    char* line = nullptr;
    size_t line_max = 0;

    auto* file = fopen(m_path.string(), "r");
    if (!file) {
        return;
    }

    String current_item;

    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, file)) != -1) {
        char* trailing_newline = strrchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        if (isspace(line[0])) {
            current_item += "\n";
            current_item += String(line + 1);
            continue;
        }

        add(move(current_item));
        current_item += String(line);
    }

    add(move(current_item));

    free(line);
    fclose(file);
}

void History::write_history() {
    auto* file = fopen(m_path.string(), "w");
    if (!file) {
        return;
    }

    for (auto& string : m_history) {
        auto lines = string.split_view('\n');
        bool first = true;
        for (auto& line : lines) {
            if (!first) {
                fputc(' ', file);
            }
            fwrite(line.data(), 1, line.size(), file);
            fputc('\n', file);
            first = false;
        }
    }

    fclose(file);
}

void History::add(String text) {
    if (text.empty() || isspace(text[0])) {
        return;
    }

    if (!m_history.empty() && m_history.last() == text) {
        return;
    }

    m_history.add(move(text));

    if (size() > max()) {
        m_history.remove(0);
    }
}

}
