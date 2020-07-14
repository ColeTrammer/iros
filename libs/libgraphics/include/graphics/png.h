#pragma once

#include <graphics/pixel_buffer.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stddef.h>
#include <stdint.h>

SharedPtr<PixelBuffer> decode_png_image(uint8_t* data, size_t data_len);
SharedPtr<PixelBuffer> decode_png_file(const String& path);
