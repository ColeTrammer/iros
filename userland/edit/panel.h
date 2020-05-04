#pragma once

#include <liim/pointers.h>

class Document;

namespace LIIM {

class String;

}

struct KeyPress;

class Panel {
public:
    virtual ~Panel();

    virtual int rows() const = 0;
    virtual int cols() const = 0;

    virtual void clear() = 0;
    virtual void set_text_at(int row, int col, char c) = 0;
    virtual void flush() = 0;
    virtual void enter() = 0;
    virtual void send_status_message(LIIM::String message) = 0;

    virtual void set_cursor(int row, int col) = 0;
    virtual int cursor_row() const = 0;
    virtual int cursor_col() const = 0;

    void set_cursor_row(int row) { set_cursor(row, cursor_col()); }
    void set_cursor_col(int col) { set_cursor(cursor_row(), col); }

    void set_document(UniquePtr<Document> document);

    Document* document() { return m_document.get(); }
    const Document* document() const { return m_document.get(); }

protected:
    Panel() {}

private:
    UniquePtr<Document> m_document;
};