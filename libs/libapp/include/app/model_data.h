#pragma once

#include <graphics/bitmap.h>
#include <graphics/text_align.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace App {

using ModelData = LIIM::Variant<Monostate, String, SharedPtr<Bitmap>, TextAlign>;

}
