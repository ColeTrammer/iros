#pragma once

#include <liim/string.h>
#include <liim/vector.h>

class Line {
public:
    Line(String contents) : m_contents(move(contents)) {}

    const String& contents() const { return m_contents; }

private:
    String m_contents;
};

class Document {
public:
    static UniquePtr<Document> create_from_file(const String& path);

    Document(Vector<Line> lines, String name) : m_lines(move(lines)), m_name(move(name)) {}

    void display() const;

private:
    Vector<Line> m_lines;
    String m_name;
};
