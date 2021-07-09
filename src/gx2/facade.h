
#pragma once

#include "gx2/surface.h"

namespace gx2 {
	void deswizzle(GX2Surface *input, GX2Surface *output);
	void decode(GX2Surface *input, GX2Surface *output);
}
