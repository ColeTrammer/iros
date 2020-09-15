#pragma once

#include <graphics/bitmap.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stdint.h>

SharedPtr<Bitmap> decode_png_image(uint8_t* data, size_t data_len);
SharedPtr<Bitmap> decode_png_file(const String& path);
