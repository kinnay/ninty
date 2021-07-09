
#include "gx2/surface.h"
#include "gx2/helpers.h"
#include "gx2/enum.h"
#include "addrinterface.h"

#include <algorithm>
#include <cstring>

using namespace gx2;


GX2TileMode default_tile_mode(GX2Surface *surface) {
	bool is_depth = surface->use & GX2_SURFACE_USE_DEPTH_BUFFER;
	bool is_color = surface->use & GX2_SURFACE_USE_COLOR_BUFFER;
	
	if (surface->dim == GX2_SURFACE_DIM_TEXTURE_1D) {
		if (surface->aa == GX2_AA_MODE_1X) {
			if (!is_depth) {
				return GX2_TILE_MODE_LINEAR_ALIGNED;
			}
		}
	}
	
	if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D && !is_color) {
		return GX2_TILE_MODE_TILED_2D_THICK;
	}
	return GX2_TILE_MODE_TILED_2D_THIN1;
}

int mip_levels_from_size(int size) {
	int mips = 0;
	while (size) {
		size /= 2;
		mips++;
	}
	return mips;
}

int calc_mip_levels(GX2Surface *surface) {
	if (surface->mip_levels <= 1) return 1;
	else {
		int levels_w = mip_levels_from_size(surface->width);
		int levels_h = mip_levels_from_size(surface->height);
		
		int levels = std::max(levels_w, levels_h);
		if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
			int levels_d = mip_levels_from_size(surface->depth);
			return std::max(levels, levels_d);
		}
		return levels;
	}
}

void *addrlib_malloc(const ADDR_ALLOCSYSMEM_INPUT *input) {
	return malloc(input->sizeInBytes);
}

ADDR_E_RETURNCODE addrlib_free(const ADDR_FREESYSMEM_INPUT *input) {
	free(input->pVirtAddr);
	return ADDR_OK;
}

ADDR_HANDLE getaddrlib() {
	static ADDR_HANDLE addrlib = NULL;
	
	if (!addrlib) {
		ADDR_CREATE_INPUT input = {};
		ADDR_CREATE_OUTPUT output = {};
		
		input.size = sizeof(input);
		input.chipEngine = CIASICIDGFXENGINE_R600;
		input.chipFamily = 0x51;
		input.chipRevision = 71;
		input.createFlags.fillSizeFields = 1;
		input.regValue.gbAddrConfig = 0x44902;
		
		input.callbacks.allocSysMem = addrlib_malloc;
		input.callbacks.freeSysMem = addrlib_free;
		
		output.size = sizeof(output);
		
		AddrCreate(&input, &output);
		
		addrlib = output.hLib;
	}
	return addrlib;
}

void get_surface_info(GX2Surface *surface, uint32_t level, ADDR_COMPUTE_SURFACE_INFO_OUTPUT *info) {
	memset(info, 0, sizeof(*info));
	info->size = sizeof(*info);
	
	uint32_t height = 1;
	if (!is_1d_dim(surface->dim)) {
		height = std::max(surface->height >> level, 1u);
	}
	
	uint32_t depth = 1;
	if (surface->dim == GX2_SURFACE_DIM_TEXTURE_CUBE) {
		depth = std::max(surface->depth, 6u);
	}
	else if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
		depth = std::max(surface->depth, 1u);
	}
	else if (is_array_dim(surface->dim)) {
		depth = surface->depth;
	}
	
	AddrFormat hw_format = (AddrFormat)(surface->format & 0x3F);
	if (surface->tile_mode == GX2_TILE_MODE_LINEAR_SPECIAL) {
		uint32_t num_samples = 1 << surface->aa;
		uint32_t block_size = is_bc_format(hw_format) ? 4 : 1;
		
		uint32_t width = ((surface->width >> level) + (block_size - 1)) & ~(block_size - 1);
		height = (height + (block_size - 1)) & ~(block_size - 1);
		
		info->bpp = surface_format_bpp[hw_format];
		info->pixelBits = surface_format_bpp[hw_format];
		info->baseAlign = 1;
		info->pitchAlign = 1;
		info->heightAlign = 1;
		info->depthAlign = 1;
		
		info->pitch = std::max(width / block_size, 1u);
		info->height = std::max(height / block_size, 1u);
		info->depth = depth;
		
		info->pixelPitch = std::max(width, block_size);
		info->pixelHeight = std::max(height, block_size);
		
		info->surfSize = (uint64_t)info->bpp * num_samples * info->depth * info->height * info->pitch / 8;
		
		if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
			info->sliceSize = info->surfSize;
		}
		else {
			info->sliceSize = info->surfSize / info->depth;
		}
		
		info->pitchTileMax = info->pitch / 8 - 1;
		info->heightTileMax = info->height / 8 - 1;
		info->sliceTileMax = info->height * info->pitch / 64 - 1;
	}
	else {
		ADDR_COMPUTE_SURFACE_INFO_INPUT input = {};
		input.size = sizeof(input);
		input.tileMode = (AddrTileMode)(surface->tile_mode & 0xF);
		input.format = hw_format;
		input.bpp = surface_format_bpp[hw_format];
		input.numSamples = 1 << surface->aa;
		input.numFrags = input.numSamples;
		
		input.width = std::max(surface->width >> level, 1u);
		input.height = height;
		input.numSlices = depth;
		
		input.slice = 0;
		input.mipLevel = level;
		
		if (surface->use & GX2_SURFACE_USE_DEPTH_BUFFER) {
			input.flags.depth = true;
		}
		if (surface->use & GX2_SURFACE_USE_SCAN_BUFFER) {
			input.flags.display = true;
		}
		if (surface->dim == GX2_SURFACE_DIM_TEXTURE_3D) {
			input.flags.volume = true;
		}
		input.flags.inputBaseMap = level == 0;
		
		AddrComputeSurfaceInfo(getaddrlib(), &input, info);
	}
}


void GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface) {
	bool tile_mode_changed = false;
	if (surface->tile_mode == GX2_TILE_MODE_DEFAULT) {
		surface->tile_mode = default_tile_mode(surface);
		if (surface->tile_mode != GX2_TILE_MODE_LINEAR_ALIGNED) {
			tile_mode_changed = true;
		}
	}
	
	surface->mip_levels = calc_mip_levels(surface);
	surface->mip_level_offset[0] = 0;
	
	surface->swizzle &= 0xFF00FFFF;
	if (is_macro_tiled(surface->tile_mode)) {
		surface->swizzle |= 0xD0000;
	}
	
	int size = 0;
	int offset = 0;
	GX2TileMode tile_mode = surface->tile_mode;
	
	ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
	for (uint32_t level = 0; level < surface->mip_levels; level++) {
		get_surface_info(surface, level, &info);
		if (level == 0) {
			if (tile_mode_changed) {
				if (surface->tile_mode != (GX2TileMode)info.tileMode) {
					surface->tile_mode = (GX2TileMode)info.tileMode;
					get_surface_info(surface, level, &info);
					if (!is_macro_tiled(surface->tile_mode)) {
						surface->swizzle &= 0xFF00FFFF;
					}
					tile_mode = surface->tile_mode;
				}
				if (surface->width < info.pitchAlign && surface->height < info.heightAlign) {
					if (surface->tile_mode == GX2_TILE_MODE_TILED_2D_THICK) {
						surface->tile_mode = GX2_TILE_MODE_TILED_1D_THICK;
					}
					else {
						surface->tile_mode = GX2_TILE_MODE_TILED_1D_THIN1;
					}
					get_surface_info(surface, level, &info);
					surface->swizzle &= 0xFF00FFFF;
					tile_mode = surface->tile_mode;
				}
			}
			surface->image_size = info.surfSize;
			surface->alignment = info.baseAlign;
			surface->pitch = info.pitch;
		}
		else {
			int pad = 0;
			if (is_macro_tiled(tile_mode) && !is_macro_tiled(info.tileMode)) {
				surface->swizzle = (surface->swizzle & 0xFF00FFFF) | (level << 16);
				if (level > 1) {
					pad = surface->swizzle & 0xFFFF;
				}
				tile_mode = (GX2TileMode)info.tileMode;
			}
			pad += (info.baseAlign - size % info.baseAlign) % info.baseAlign;
			if (level == 1) {
				offset = size + pad;
			}
			else {
				surface->mip_level_offset[level - 1] = size + pad + surface->mip_level_offset[level - 2];
			}
		}
		size = info.surfSize;
	}
	
	surface->mipmap_size = 0;
	if (surface->mip_levels > 1) {
		surface->mipmap_size = size + surface->mip_level_offset[surface->mip_levels - 2];
	}
	surface->mip_level_offset[0] = offset;
	
	if (surface->format == GX2_SURFACE_FORMAT_UNORM_NV12) {
		int pad = (surface->alignment - surface->image_size % surface->alignment) % surface->alignment;
		surface->mip_level_offset[0] = surface->image_size + pad;
		surface->image_size = surface->mip_level_offset[0] + surface->image_size / 2;
	}
}

void init_addrfromcoord_info(
	GX2Surface *surface, uint32_t slice,
	ADDR_COMPUTE_SURFACE_INFO_OUTPUT *surface_info,
	ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *info
) {
	ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT swizzle_in;
	ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT swizzle_out;
	
	swizzle_in.size = sizeof(swizzle_in);
	swizzle_in.base256b = (surface->swizzle >> 8) & 0xFF;
	swizzle_out.size = sizeof(swizzle_out);
	AddrExtractBankPipeSwizzle(getaddrlib(), &swizzle_in, &swizzle_out);
	
	info->size = sizeof(*info);
	info->slice = slice;
	info->sample = 0;
	info->bpp = surface_format_bpp[surface->format & 0x3F];
	info->pitch = surface_info->pitch;
	info->height = surface_info->height;
	info->numSlices = std::max(surface_info->depth, 1u);
	info->numSamples = 1 << surface->aa;
	info->tileMode = surface_info->tileMode;
	info->isDepth = surface->use & GX2_SURFACE_USE_DEPTH_BUFFER;
	info->tileBase = 0;
	info->compBits = 0;
	info->pipeSwizzle = swizzle_out.pipeSwizzle;
	info->bankSwizzle = swizzle_out.bankSwizzle;
	info->numFrags = 0;
}

void GX2CopySurface(
	GX2Surface *src, uint32_t src_level, uint32_t src_slice,
	GX2Surface *dst, uint32_t dst_level, uint32_t dst_slice
) {
	ADDR_COMPUTE_SURFACE_INFO_OUTPUT src_info;
	ADDR_COMPUTE_SURFACE_INFO_OUTPUT dst_info;
	get_surface_info(src, src_level, &src_info);
	get_surface_info(dst, dst_level, &dst_info);
	
	ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT src_addr_in = {};
	ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT dst_addr_in = {};
	ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT src_addr_out = {};
	ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT dst_addr_out = {};
	
	init_addrfromcoord_info(src, src_slice, &src_info, &src_addr_in);
	init_addrfromcoord_info(dst, dst_slice, &dst_info, &dst_addr_in);
	src_addr_out.size = sizeof(src_addr_out);
	dst_addr_out.size = sizeof(dst_addr_out);
	
	uint32_t src_width = std::max(src->width >> src_level, 1u);
	uint32_t src_height = std::max(src->height >> src_level, 1u);
	if (is_bc_format(src->format)) {
		src_width = (src_width + 3) / 4;
		src_height = (src_height + 3) / 4;
	}
	
	uint32_t dst_width = std::max(dst->width >> dst_level, 1u);
	uint32_t dst_height = std::max(dst->height >> dst_level, 1u);
	if (is_bc_format(dst->format)) {
		dst_width = (dst_width + 3) / 4;
		dst_height = (dst_height + 3) / 4;
	}
	
	uint8_t *src_base = get_mipmap_ptr(src, src_level);
	uint8_t *dst_base = get_mipmap_ptr(dst, dst_level);
	
	int bpp = surface_format_bpp[src->format & 0x3F];
	for (uint32_t y = 0; y < dst_height; y++) {
		for (uint32_t x = 0; x < dst_width; x++) {
			dst_addr_in.x = x;
			dst_addr_in.y = y;
			src_addr_in.x = src_width * x / dst_width;
			src_addr_in.y = src_height * y / dst_height;
			
			if (src->tile_mode == GX2_TILE_MODE_LINEAR_SPECIAL) {
				src_addr_out.addr = (x + y * src_info.pitch + src_slice * src_height * src_info.pitch) * bpp / 8;
			}
			else {
				AddrComputeSurfaceAddrFromCoord(getaddrlib(), &src_addr_in, &src_addr_out);
			}
			
			if (dst->tile_mode == GX2_TILE_MODE_LINEAR_SPECIAL) {
				dst_addr_out.addr = (x + y * dst_info.pitch + dst_slice * dst_height * dst_info.pitch) * bpp / 8;
			}
			else {
				AddrComputeSurfaceAddrFromCoord(getaddrlib(), &dst_addr_in, &dst_addr_out);
			}
			
			void *src_ptr = src_base + src_addr_out.addr;
			void *dst_ptr = dst_base + dst_addr_out.addr;
			memcpy(dst_ptr, src_ptr, bpp / 8);
		}
	}
}
