
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <cstring>

enum GX2SurfaceFormat {
	GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 = 0x1A,
	GX2_SURFACE_FORMAT_UNORM_BC1 = 0x31,
	GX2_SURFACE_FORMAT_UNORM_BC3 = 0x33,
	GX2_SURFACE_FORMAT_UNORM_BC5 = 0x35
};

enum GX2TileMode {
	GX2_TILE_MODE_TILED_1D_THIN1 = 2,
	GX2_TILE_MODE_TILED_2D_THIN1 = 4,
	GX2_TILE_MODE_LINEAR_SPECIAL = 16
};


enum class GX2Error {
	OK,
	ImageTooBig,
	ImageNotAligned,
	ImageSizeMismatch,
	FormatNotSupported,
	TileModeNotSupported
};


bool check_image_size(uint32_t width, uint32_t height) {
	return width < 0x4000 && height < 0x4000;
}

bool check_image_format(GX2SurfaceFormat format) {
	return format == GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8 ||
		format == GX2_SURFACE_FORMAT_UNORM_BC1 ||
		format == GX2_SURFACE_FORMAT_UNORM_BC3 ||
		format == GX2_SURFACE_FORMAT_UNORM_BC5;
}

bool check_tile_mode(uint32_t tilemode) {
	return tilemode == GX2_TILE_MODE_TILED_1D_THIN1 ||
		tilemode == GX2_TILE_MODE_TILED_2D_THIN1 ||
		tilemode == GX2_TILE_MODE_LINEAR_SPECIAL;
}

bool is_compressed(uint32_t format) {
	return format == GX2_SURFACE_FORMAT_UNORM_BC1 ||
		format == GX2_SURFACE_FORMAT_UNORM_BC3 ||
		format == GX2_SURFACE_FORMAT_UNORM_BC5;
}

uint32_t bytes_per_pixel(uint32_t format) {
	switch(format) {
		case GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8: return 4;
		case GX2_SURFACE_FORMAT_UNORM_BC1: return 8;
		case GX2_SURFACE_FORMAT_UNORM_BC3: return 16;
		case GX2_SURFACE_FORMAT_UNORM_BC5: return 16;
		default: return 0;
	}
}

uint32_t get_block_width(uint32_t tilemode) {
	switch (tilemode) {
		case GX2_TILE_MODE_TILED_1D_THIN1: return 8;
		case GX2_TILE_MODE_TILED_2D_THIN1: return 32;
		default: return 0;
	}
}

uint32_t get_block_height(uint32_t tilemode) {
	switch (tilemode) {
		case GX2_TILE_MODE_TILED_1D_THIN1: return 8;
		case GX2_TILE_MODE_TILED_2D_THIN1: return 16;
		default: return 0;
	}
}

GX2Error deswizzle_surface(
	const uint8_t *in, size_t inlen, uint8_t *out,
	uint32_t width, uint32_t height, GX2SurfaceFormat format,
	GX2TileMode tilemode, uint32_t swizzle
) {
	if (tilemode == GX2_TILE_MODE_LINEAR_SPECIAL) {
		memcpy(out, in, inlen);
		return GX2Error::OK;
	}
	
	if (!check_image_size(width, height)) return GX2Error::ImageTooBig;
	if (!check_image_format(format)) return GX2Error::FormatNotSupported;
	if (!check_tile_mode(tilemode)) return GX2Error::TileModeNotSupported;
	
	if (is_compressed(format)) {
		width /= 4;
		height /= 4;
	}
	
	swizzle = swizzle & 7;
	
	uint32_t pixelsize = bytes_per_pixel(format);
	uint32_t tilesize = pixelsize * 64;
	
	if (inlen < width * height * pixelsize) {
		return GX2Error::ImageSizeMismatch;
	}
	
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint32_t mask = 16 / pixelsize - 1;
			uint32_t tempidx = ((y & 6) << 2) | (x & 7);
			uint32_t pixelidx = (tempidx & mask) | ((tempidx & ~mask) << 1) | ((y & 1) * (mask + 1));
			uint32_t pixeloffs = pixelidx * pixelsize;
			
			uint32_t tiledoffs = 0;
			if (tilemode == 2) {
				uint32_t tilerow = width / 8;
				uint32_t tilex = x / 8;
				uint32_t tiley = y / 8;
				uint32_t tileoffs = (tilex + tiley * tilerow) * tilesize;
				tiledoffs = tileoffs + pixeloffs;
			}
			else if (tilemode == 4) {
				uint32_t bankpipe = ((y >> 3) ^ (x >> 3)) & 1;
				bankpipe |= ((y >> 4) ^ (x >> 2)) & 2;
				bankpipe |= ((y >> 2) ^ (x >> 2)) & 4;
				bankpipe ^= swizzle;
				
				uint32_t blockw = 32;
				uint32_t blockh = 16;
				uint32_t blockx = x / blockw;
				uint32_t blocky = y / blockh;
				uint32_t blocksize = pixelsize * blockh * blockw;
				uint32_t blockoffs = (blockx + blocky * (width / blockw)) * blocksize;
				
				uint32_t totaloffs = pixeloffs + (blockoffs >> 3);
				tiledoffs = (bankpipe << 8) | (totaloffs & 0xFF) | ((totaloffs & ~0xFF) << 3);
			}
			
			uint32_t linearoffs = (y * width + x) * pixelsize;
			memcpy(out + linearoffs, in + tiledoffs, pixelsize);
		}
	}
	return GX2Error::OK;
}

void bc_interpolate(uint8_t *colors, int count) {
	for (int i = 1; i < count; i++) {
		colors[i+1] = (colors[0] * (count - i) + colors[1] * i) / count;
	}
}

void bc1_interpolate(bool mode, uint8_t *colors) {
	if (mode) {
		bc_interpolate(colors, 3);
	}
	else {
		bc_interpolate(colors, 2);
		colors[3] = 0;
	}
}

void bc3_interpolate(uint8_t *colors) {
	if (colors[0] > colors[1]) {
		bc_interpolate(colors, 7);
	}
	else {
		bc_interpolate(colors, 5);
		colors[5] = 0;
		colors[6] = 0xFF;
	}
}

void bc_decode_565(uint16_t color, uint8_t *red, uint8_t *green, uint8_t *blue) {
	red[0] = (color >> 11) << 3;
	green[0] = (color >> 3) & 0xFC;
	blue[0] = (color & 0x1F) << 3;
}

GX2Error decompress_bc1(const uint8_t *in, size_t inlen, uint8_t *out, uint32_t width, uint32_t height) {
	if (width % 4 || height % 4) {
		return GX2Error::ImageNotAligned;
	}
	if (inlen < width * height / 2) {
		return GX2Error::ImageSizeMismatch;
	}
	for (uint32_t by = 0; by < height; by += 4) {
		for (uint32_t bx = 0; bx < width; bx += 4) {
			uint16_t color0 = in[0] | (in[1] << 8);
			uint16_t color1 = in[2] | (in[3] << 8);
			in += 4;
			
			uint8_t red[4], green[4], blue[4];
			bc_decode_565(color0, red, green, blue);
			bc_decode_565(color1, red+1, green+1, blue+1);
			
			uint8_t alpha[4];
			alpha[0] = 0xFF;
			alpha[1] = 0xFF;
			
			bc1_interpolate(color0 > color1, red);
			bc1_interpolate(color0 > color1, green);
			bc1_interpolate(color0 > color1, blue);
			bc1_interpolate(color0 > color1, alpha);
			
			for (int y = 0; y < 4; y++) {
				uint32_t offset = ((by + y) * width + bx) * 4;
				uint8_t pixels = in[0];
				for (int x = 0; x < 4; x++) {
					out[offset] = red[pixels & 3];
					out[offset+1] = green[pixels & 3];
					out[offset+2] = blue[pixels & 3];
					out[offset+3] = alpha[pixels & 3];
					offset += 4;
					pixels >>= 2;
				}
				in += 1;
			}
		}
	}
	return GX2Error::OK;
}

GX2Error decompress_bc3(const uint8_t *in, size_t inlen, uint8_t *out, uint32_t width, uint32_t height) {
	if (width % 4 || height % 4) {
		return GX2Error::ImageNotAligned;
	}
	if (inlen < width * height) {
		return GX2Error::ImageSizeMismatch;
	}
	for (uint32_t by = 0; by < height; by += 4) {
		for (uint32_t bx = 0; bx < width; bx += 4) {
			uint8_t alpha[8];
			alpha[0] = in[0];
			alpha[1] = in[1];
			
			bc3_interpolate(alpha);
			
			uint16_t color0 = in[8] | (in[9] << 8);
			uint16_t color1 = in[10] | (in[11] << 8);
			
			uint8_t red[4], green[4], blue[4];
			bc_decode_565(color0, red, green, blue);
			bc_decode_565(color1, red+1, green+1, blue+1);
			
			bc1_interpolate(true, red);
			bc1_interpolate(true, green);
			bc1_interpolate(true, blue);
			
			uint32_t alphabits = in[2] | (in[3] << 8) | (in[4] << 16);
			uint32_t alphabits2 = in[5] | (in[6] << 8) | (in[7] << 16);
			
			for (int y = 0; y < 4; y++) {
				if (y == 2) {
					alphabits = alphabits2;
				}
				
				size_t offset = ((by + y) * width + bx) * 4;
				uint8_t colorbits = in[y + 12];
				for (int x = 0; x < 4; x++) {
					out[offset] = red[colorbits & 3];
					out[offset+1] = green[colorbits & 3];
					out[offset+2] = blue[colorbits & 3];
					out[offset+3] = alpha[alphabits & 7];
					offset += 4;
					alphabits >>= 3;
					colorbits >>= 2;
				}
			}
			
			in += 16;
		}
	}
	return GX2Error::OK;
}

GX2Error decompress_bc5(const uint8_t *in, size_t inlen, uint8_t *out, uint32_t width, uint32_t height) {
	if (width % 4 || height % 4) {
		return GX2Error::ImageNotAligned;
	}
	if (inlen < width * height) {
		return GX2Error::ImageSizeMismatch;
	}
	for (uint32_t by = 0; by < height; by += 4) {
		for (uint32_t bx = 0; bx < width; bx += 4) {
			uint8_t red[8], green[8];
			red[0] = in[0];
			red[1] = in[1];
			green[0] = in[8];
			green[1] = in[9];
			
			bc3_interpolate(red);
			bc3_interpolate(green);
			
			uint32_t redbits = in[2] | (in[3] << 8) | (in[4] << 16);
			uint32_t redbits2 = in[5] | (in[6] << 8) | (in[7] << 16);
			uint32_t greenbits = in[10] | (in[11] << 8) | (in[12] << 16);
			uint32_t greenbits2 = in[13] | (in[14] << 8) | (in[15] << 16);
			
			for (int y = 0; y < 4; y++) {
				if (y == 2) {
					redbits = redbits2;
					greenbits = greenbits2;
				}
				
				size_t offset = ((by + y) * width + bx) * 4;
				for (int x = 0; x < 4; x++) {
					out[offset] = red[redbits & 7];
					out[offset+1] = green[greenbits & 7];
					out[offset+2] = 0;
					out[offset+3] = 0xFF;
					offset += 4;
					redbits >>= 3;
					greenbits >>= 3;
				}
			}
			
			in += 16;
		}
	}
	return GX2Error::OK;
}

GX2Error decode_image(const uint8_t *in, size_t inlen, uint8_t *out, uint32_t width, uint32_t height, GX2SurfaceFormat format) {
	if (format == GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8) {
		if (inlen < width * height * 4) {
			return GX2Error::ImageSizeMismatch;
		}
		memcpy(out, in, width * height * 4);
		return GX2Error::OK;
	}
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC1) return decompress_bc1(in, inlen, out, width, height);
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC3) return decompress_bc3(in, inlen, out, width, height);
	else if (format == GX2_SURFACE_FORMAT_UNORM_BC5) return decompress_bc5(in, inlen, out, width, height);
	return GX2Error::FormatNotSupported;
}


void GX2_set_error(GX2Error error) {
	if (error == GX2Error::ImageTooBig) {
		PyErr_SetString(PyExc_OverflowError, "image too large");
	}
	else if (error == GX2Error::ImageNotAligned) {
		PyErr_SetString(PyExc_ValueError, "image must be dividle in 4x4 blocks");
	}
	else if (error == GX2Error::ImageSizeMismatch) {
		PyErr_SetString(PyExc_ValueError, "image buffer is too small for specified dimensions");
	}
	else if (error == GX2Error::FormatNotSupported) {
		PyErr_SetString(PyExc_ValueError, "image format not supported");
	}
	else if (error == GX2Error::TileModeNotSupported) {
		PyErr_SetString(PyExc_ValueError, "tile mode not supported");
	}
}

PyObject *GX2_deswizzle(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t width;
	uint32_t height;
	GX2SurfaceFormat format;
	GX2TileMode tilemode;
	uint32_t swizzle;
	
	if (!PyArg_ParseTuple(
	  args, "y#IIIII", &in, &inlen, &width, &height,
	  &format, &tilemode, &swizzle
	)) {
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, inlen);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	GX2Error error = deswizzle_surface(in, inlen, out, width, height, format, tilemode, swizzle);
	if (error != GX2Error::OK) {
		Py_DECREF(bytes);
		GX2_set_error(error);
		return NULL;
	}
	return bytes;
}

PyObject *GX2_decode(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t width;
	uint32_t height;
	GX2SurfaceFormat format;
	if (!PyArg_ParseTuple(args, "y#III", &in, &inlen, &width, &height, &format)) {
		return NULL;
	}
	
	if (!check_image_size(width, height)) {
		GX2_set_error(GX2Error::ImageTooBig);
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, width * height * 4);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	GX2Error error = decode_image(in, inlen, out, width, height, format);
	if (error != GX2Error::OK) {
		Py_DECREF(bytes);
		GX2_set_error(error);
		return NULL;
	}
	return bytes;
}

PyMethodDef GX2Methods[] = {
	{"deswizzle", GX2_deswizzle, METH_VARARGS, NULL},
	{"decode", GX2_decode, METH_VARARGS, NULL},
	NULL
};

PyModuleDef GX2Module = {
	PyModuleDef_HEAD_INIT,
	"gx2",
	"Texture methods",
	-1,
	
	GX2Methods
};

PyMODINIT_FUNC PyInit_gx2() {
	return PyModule_Create(&GX2Module);
}
