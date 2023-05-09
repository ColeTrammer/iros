#pragma once

#include <di/io/interface/writer.h>
#include <di/io/read_all.h>
#include <di/io/read_to_string.h>
#include <di/io/string_reader.h>
#include <di/io/string_writer.h>
#include <di/io/write_exactly.h>
#include <di/io/writer_print.h>
#include <di/io/writer_println.h>

namespace di {
using io::Reader;
using io::StringReader;
using io::StringWriter;
using io::Writer;

using io::read_all;
using io::read_to_string;
using io::write_exactly;
using io::writer_print;
using io::writer_println;
}
