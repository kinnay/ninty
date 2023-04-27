
#pragma once

#include "gx2/enum.h"
#include <cstddef>
#include <cstdint>

namespace gx2 {

void decode_pixels(uint8_t *dst, const uint8_t *src, uint32_t width, uint32_t height, GX2SurfaceFormat format);
bool is_format_supported(GX2SurfaceFormat format);

extern GX2SurfaceFormat supported_formats[];
extern size_t num_supported_formats;

}
