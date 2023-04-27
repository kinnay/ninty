
#include "gx2/facade.h"
#include "gx2/surface.h"
#include "gx2/helpers.h"
#include "gx2/codecs.h"
#include <algorithm>
#include <cstdlib>


bool gx2::convert_tilemode(GX2Surface *input, GX2Surface *output, GX2TileMode tilemode, uint8_t swizzle) {
	*output = *input;
	output->tile_mode = tilemode;
	output->swizzle = (output->swizzle & 0xFFFF00FF) | (swizzle << 8);
	GX2CalcSurfaceSizeAndAlignment(output);
	
	output->image = (uint8_t *)malloc(output->image_size + output->mipmap_size);
	if (!output->image) return false;
	
	output->mipmaps = nullptr;
	if (output->mip_levels > 1) {
		output->mipmaps = output->image + output->image_size;
	}
	
	for (uint32_t level = 0; level < input->mip_levels; level++) {
		uint32_t depth = input->depth;
		if (input->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
			depth = std::max(depth >> level, 1u);
		}
		for (uint32_t slice = 0; slice < depth; slice++) {
			GX2CopySurface(input, level, slice, output, level, slice);
		}
	}
	
	return true;
}

bool gx2::decode(GX2Surface *input, GX2Surface *output) {
	*output = *input;
	output->format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
	GX2CalcSurfaceSizeAndAlignment(output);
	
	output->image = (uint8_t *)malloc(output->image_size + output->mipmap_size);
	if (!output->image) return false;
	
	output->mipmaps = nullptr;
	if (output->mip_levels > 1) {
		output->mipmaps = output->image + output->image_size;
	}
	
	for (uint32_t level = 0; level < input->mip_levels; level++) {
		uint32_t depth = input->depth;
		if (input->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
			depth = std::max(depth >> level, 1u);
		}
		
		for (uint32_t slice = 0; slice < depth; slice++) {
			uint8_t *src = get_mipmap_ptr(input, level, slice);
			uint8_t *dst = get_mipmap_ptr(output, level, slice);
			
			uint32_t width = std::max(input->width >> level, 1u);
			uint32_t height = std::max(input->height >> level, 1u);
			decode_pixels(dst, src, width, height, input->format);
		}
	}
	return true;
}
