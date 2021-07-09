
#pragma once

#include "gx2/enum.h"
#include <cstdint>

namespace gx2 {

void decode_pixels(uint8_t *dst, const uint8_t *src, uint32_t width, uint32_t height, GX2SurfaceFormat format);

}
