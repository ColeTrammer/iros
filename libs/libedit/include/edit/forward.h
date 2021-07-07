#pragma once

#include <stddef.h>

namespace Edit {
class CharacterMetadata;
class Command;
class Cursor;
class Document;
class KeyPress;
class Line;
class MultiCursor;
class Panel;
class Selection;
class Suggestions;
class TextIndex;
class TextRangeCollectionIterator;
class TextRangeCollection;

template<size_t N>
class TextRangeCombinerIterator;

class TextRange;
enum class DocumentType;
struct LineSplitResult;
struct Position;

using DocumentTextRangeIterator = TextRangeCombinerIterator<3>;
}
