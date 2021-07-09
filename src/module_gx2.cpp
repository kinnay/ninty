
#define PY_SSIZE_T_CLEAN

#include "gx2/facade.h"
#include "gx2/surface.h"
#include "gx2/enum.h"

#include <Python.h>
#include <cstdint>
#include <cstring>


GX2SurfaceFormat supported_formats[] = {
	GX2_SURFACE_FORMAT_UNORM_R8_G8,
	GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
	GX2_SURFACE_FORMAT_UNORM_BC1,
	GX2_SURFACE_FORMAT_UNORM_BC3,
	GX2_SURFACE_FORMAT_UNORM_BC5
};


PyObject *GX2_deswizzle(PyObject *self, PyObject *args) {
	uint8_t *in;
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
	
	GX2Surface surface;
	surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
	surface.width = width;
	surface.height = height;
	surface.depth = 1;
	surface.mip_levels = 1;
	surface.format = format;
	surface.aa = GX2_AA_MODE_1X;
	surface.use = GX2_SURFACE_USE_TEXTURE;
	surface.tile_mode = tilemode;
	surface.swizzle = swizzle << 8;
	
	GX2CalcSurfaceSizeAndAlignment(&surface);
	
	if (inlen < surface.image_size + surface.mipmap_size) {
		PyErr_SetString(PyExc_ValueError, "image buffer is too small for specified dimensions");
		return NULL;
	}
	
	surface.image = in;
	if (surface.mip_levels > 1) {
		surface.mipmaps = surface.image + surface.image_size;
	}
	
	GX2Surface output;
	gx2::deswizzle(&surface, &output);
	
	PyObject *bytes = PyBytes_FromStringAndSize((char *)output.image, output.image_size + output.mipmap_size);
	free(output.image);
	return bytes;
}

PyObject *GX2_decode(PyObject *self, PyObject *args) {
	uint8_t *in;
	size_t inlen;
	uint32_t width;
	uint32_t height;
	GX2SurfaceFormat format;
	if (!PyArg_ParseTuple(args, "y#III", &in, &inlen, &width, &height, &format)) {
		return NULL;
	}
	
	bool supported = false;
	for (size_t i = 0; i < sizeof(supported_formats) / sizeof(supported_formats[0]); i++) {
		if (supported_formats[i] == format) {
			supported = true;
			break;
		}
	}
	
	if (!supported) {
		PyErr_SetString(PyExc_ValueError, "image format not supported");
		return NULL;
	}
	
	GX2Surface surface;
	surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
	surface.width = width;
	surface.height = height;
	surface.depth = 1;
	surface.mip_levels = 1;
	surface.format = format;
	surface.aa = GX2_AA_MODE_1X;
	surface.use = GX2_SURFACE_USE_TEXTURE;
	surface.tile_mode = GX2_TILE_MODE_LINEAR_SPECIAL;
	surface.swizzle = 0;
	
	GX2CalcSurfaceSizeAndAlignment(&surface);
	
	if (inlen < surface.image_size + surface.mipmap_size) {
		PyErr_SetString(PyExc_ValueError, "image buffer is too small for specified dimensions");
		return NULL;
	}
	
	surface.image = in;
	if (surface.mip_levels > 1) {
		surface.mipmaps = surface.image + surface.image_size;
	}
	
	GX2Surface output;
	gx2::decode(&surface, &output);
	
	PyObject *bytes = PyBytes_FromStringAndSize((char *)output.image, output.image_size + output.mipmap_size);
	free(output.image);
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
