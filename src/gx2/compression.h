
#pragma once

#include <cstdint>

namespace gx2 {

void decompress_bc1(uint8_t *out, const uint8_t *in);
void decompress_bc3(uint8_t *out, const uint8_t *in);
void decompress_bc5(uint8_t *out, const uint8_t *in);

}
