
#include "gx2/helpers.h"
#include <algorithm>

namespace gx2 {

const int surface_format_bpp[] = {
	0, 8, 8, 0, 0, 16, 16, 16,
	16, 16, 16, 16, 16, 32, 32, 32,
	32, 32, 0, 32, 0, 0, 32, 0,
	0, 32, 32, 32, 64, 64, 64, 64,
	64, 0, 128, 128, 0, 0, 0, 16,
	16, 32, 32, 32, 0, 0, 0, 96,
	96, 64, 128, 128, 64, 128, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

bool is_macro_tiled(GX2TileMode tile_mode) {
	return tile_mode >= GX2_TILE_MODE_TILED_2D_THIN1 && tile_mode != GX2_TILE_MODE_LINEAR_SPECIAL;
}

bool is_macro_tiled(AddrTileMode tile_mode) {
	return is_macro_tiled((GX2TileMode)tile_mode);
}

bool is_bc_format(AddrFormat format) {
	return format >= ADDR_FMT_BC1 && format <= ADDR_FMT_BC5;
}

bool is_bc_format(GX2SurfaceFormat format) {
	return is_bc_format((AddrFormat)(format & 0x3F));
}

bool is_1d_dim(GX2SurfaceDim dim) {
	return dim == GX2_SURFACE_DIM_TEXTURE_1D || dim == GX2_SURFACE_DIM_TEXTURE_1D_ARRAY;
}

bool is_array_dim(GX2SurfaceDim dim) {
	return dim == GX2_SURFACE_DIM_TEXTURE_1D_ARRAY ||
	       dim == GX2_SURFACE_DIM_TEXTURE_2D_ARRAY ||
	       dim == GX2_SURFACE_DIM_TEXTURE_2D_MSAA_ARRAY;
}

uint8_t *get_mipmap_ptr(GX2Surface *surface, uint32_t level, uint32_t slice) {
	uint32_t depth = surface->depth;
	if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
		depth = std::max(depth >> level, 1u);
	}
	
	uint8_t *base;
	uint32_t size;
	if (level == 0) {
		base = surface->image;
		size = surface->image_size;
	}
	else if (level == 1) {
		base = surface->mipmaps;
		size = surface->mipmap_size;
		if (surface->mip_levels > 2) {
			size = surface->mip_level_offset[1];
		}
	}
	else {
		base = surface->mipmaps + surface->mip_level_offset[level - 1];
		if (surface->mip_levels - 1 > level) {
			size = surface->mip_level_offset[level] - surface->mip_level_offset[level - 1];
		}
		else {
			size = surface->mipmap_size - surface->mip_level_offset[level - 1];
		}
	}
	
	return base + size / surface->depth * slice;
}

}
