
#pragma once

#include "gx2/enum.h"
#include <cstdint>

struct GX2Surface {
	GX2SurfaceDim dim;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mip_levels;
	GX2SurfaceFormat format;
	GX2AAMode aa;
	GX2SurfaceUse use;
	uint32_t image_size;
	uint8_t *image;
	uint32_t mipmap_size;
	uint8_t *mipmaps;
	GX2TileMode tile_mode;
	uint32_t swizzle;
	uint32_t alignment;
	uint32_t pitch;
	uint32_t mip_level_offset[13];
};

void GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface);
void GX2CopySurface(
	GX2Surface *src, uint32_t src_level, uint32_t src_slice,
	GX2Surface *dst, uint32_t dst_level, uint32_t dst_slice
);
