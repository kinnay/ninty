
#pragma once

#include "gx2/surface.h"

namespace gx2 {
	bool convert_tilemode(GX2Surface *input, GX2Surface *output, GX2TileMode tilemode, uint8_t swizzle);
	bool decode(GX2Surface *input, GX2Surface *output);
}
