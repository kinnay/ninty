
#pragma once

#include "addrinterface.h"
#include "gx2/enum.h"
#include "gx2/surface.h"

namespace gx2 {
	
extern const int surface_format_bpp[];

bool is_macro_tiled(GX2TileMode tile_mode);
bool is_macro_tiled(AddrTileMode tile_mode);

bool is_bc_format(GX2SurfaceFormat format);
bool is_bc_format(AddrFormat format);

bool is_1d_dim(GX2SurfaceDim dim);
bool is_array_dim(GX2SurfaceDim dim);

uint8_t *get_mipmap_ptr(GX2Surface *surface, uint32_t level, uint32_t slice = 0);

}
