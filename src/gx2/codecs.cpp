
#include "gx2/codecs.h"
#include "gx2/compression.h"
#include "gx2/helpers.h"

namespace gx2 {

GX2SurfaceFormat supported_formats[] = {
	GX2_SURFACE_FORMAT_UNORM_R8_G8,
	GX2_SURFACE_FORMAT_UNORM_R5_G6_B5,
	GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
	GX2_SURFACE_FORMAT_UNORM_BC1,
	GX2_SURFACE_FORMAT_UNORM_BC3,
	GX2_SURFACE_FORMAT_UNORM_BC4,
	GX2_SURFACE_FORMAT_UNORM_BC5
};

size_t num_supported_formats = sizeof(supported_formats) / sizeof(supported_formats[0]);

bool is_format_supported(GX2SurfaceFormat format) {
	for (size_t i = 0; i < num_supported_formats; i++) {
		if (supported_formats[i] == format) return true;
	}
	return false;
}

uint32_t pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t decode_pixel(const void *pix, GX2SurfaceFormat format) {
	if (format == GX2_SURFACE_FORMAT_UNORM_R8_G8) {
		uint8_t *ptr = (uint8_t *)pix;
		return pixel(ptr[0], ptr[1], 0, 0xFF);
	}
	else if (format == GX2_SURFACE_FORMAT_UNORM_R5_G6_B5) {
		uint16_t value = *(uint16_t *)pix;
		uint8_t r = value >> 11;
		uint8_t g = (value >> 5) & 0x3F;
		uint8_t b = value & 0x1F;
		return pixel(r << 3, g << 2, b << 3, 0xFF);
	}
	else if (format == GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8) {
		uint8_t *ptr = (uint8_t *)pix;
		return pixel(ptr[0], ptr[1], ptr[2], ptr[3]);
	}
	return 0;
}

void decompress_block(uint8_t *block, const uint8_t *pix, GX2SurfaceFormat format) {
	if (format == GX2_SURFACE_FORMAT_UNORM_BC1) decompress_bc1(block, pix);
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC3) decompress_bc3(block, pix);
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC4) decompress_bc4(block, pix);
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC5) decompress_bc5(block, pix);
}

void decompress_pixels(uint8_t *dst, const uint8_t *src, uint32_t width, uint32_t height, GX2SurfaceFormat format) {
	int bpp = surface_format_bpp[format & 0x3F];
	for (uint32_t by = 0; by < height; by += 4) {
		for (uint32_t bx = 0; bx < width; bx += 4) {
			uint8_t block[64];
			decompress_block(block, src, format);
			
			for (int y = 0; y < 4; y++) {
				if (by + y >= height) break;
				
				size_t offset = ((by + y) * width + bx) * 4;
				for (int x = 0; x < 4; x++) {
					if (bx + x >= width) break;
					
					uint8_t *ptr = block + (y * 4 + x) * 4;
					dst[offset] = ptr[0];
					dst[offset+1] = ptr[1];
					dst[offset+2] = ptr[2];
					dst[offset+3] = ptr[3];
					offset += 4;
				}
			}
			src += bpp / 8;
		}
	}
}

void decode_pixels(uint8_t *dst, const uint8_t *src, uint32_t width, uint32_t height, GX2SurfaceFormat format) {
	if (is_bc_format(format)) decompress_pixels(dst, src, width, height, format);
	else {
		int bpp = surface_format_bpp[format & 0x3F];
		for (uint32_t y = 0; y < height; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint32_t pixel = decode_pixel(src, format);
				dst[0] = pixel >> 24;
				dst[1] = (pixel >> 16) & 0xFF;
				dst[2] = (pixel >> 8) & 0xFF;
				dst[3] = pixel & 0xFF;
				src += bpp / 8;
				dst += 4;
			}
		}
	}
}

}
