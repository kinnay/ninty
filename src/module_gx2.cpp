
#define PY_SSIZE_T_CLEAN

#include "type_surface.h"

#include "gx2/facade.h"
#include "gx2/surface.h"
#include "gx2/codecs.h"
#include "gx2/enum.h"

#include <Python.h>
#include <cstdint>
#include <cstring>

PyObject *convert_tilemode(
	uint8_t *in, size_t inlen, uint32_t width, uint32_t height, GX2SurfaceFormat format,
	GX2TileMode tilemode_in, uint8_t swizzle_in, GX2TileMode tilemode_out, uint8_t swizzle_out
) {
	GX2Surface surface;
	surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
	surface.width = width;
	surface.height = height;
	surface.depth = 1;
	surface.mip_levels = 1;
	surface.format = format;
	surface.aa = GX2_AA_MODE_1X;
	surface.use = GX2_SURFACE_USE_TEXTURE;
	surface.tile_mode = tilemode_in;
	surface.swizzle = swizzle_in << 8;
	
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
	if (!gx2::convert_tilemode(&surface, &output, tilemode_out, swizzle_out)) {
		return PyErr_NoMemory();
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize((char *)output.image, output.image_size + output.mipmap_size);
	free(output.image);
	return bytes;
}

PyObject *GX2_swizzle(PyObject *self, PyObject *args) {
	uint8_t *in;
	size_t inlen;
	uint32_t width;
	uint32_t height;
	GX2SurfaceFormat format;
	GX2TileMode tilemode;
	uint8_t swizzle;
	
	if (!PyArg_ParseTuple(
	  args, "y#IIIIb", &in, &inlen, &width, &height,
	  &format, &tilemode, &swizzle
	)) {
		return NULL;
	}
	
	return convert_tilemode(
		in, inlen, width, height, format,
		GX2_TILE_MODE_LINEAR_SPECIAL, 0, tilemode, swizzle
	);
}

PyObject *GX2_deswizzle(PyObject *self, PyObject *args) {
	uint8_t *in;
	size_t inlen;
	uint32_t width;
	uint32_t height;
	GX2SurfaceFormat format;
	GX2TileMode tilemode;
	uint8_t swizzle;
	
	if (!PyArg_ParseTuple(
	  args, "y#IIIIb", &in, &inlen, &width, &height,
	  &format, &tilemode, &swizzle
	)) {
		return NULL;
	}
	
	return convert_tilemode(
		in, inlen, width, height, format,
		tilemode, swizzle, GX2_TILE_MODE_LINEAR_SPECIAL, 0
	);
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
	if (!gx2::is_format_supported(format)) {
		PyErr_SetString(PyExc_ValueError, "surface format not supported");
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
	if (!gx2::decode(&surface, &output)) {
		return PyErr_NoMemory();
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize((char *)output.image, output.image_size + output.mipmap_size);
	free(output.image);
	return bytes;
}

PyMethodDef GX2Methods[] = {
	{"swizzle", GX2_swizzle, METH_VARARGS, NULL},
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
	PyObject *module = PyModule_Create(&GX2Module);
	if (!module) return NULL;
	
	PyObject *formats = PyList_New(gx2::num_supported_formats);
	for (size_t i = 0; i < gx2::num_supported_formats; i++) {
		PyList_SetItem(formats, i, PyLong_FromLong(gx2::supported_formats[i]));
	}
	
	if (PyModule_AddObject(module, "SUPPORTED_SURFACE_FORMATS", formats) < 0) {
		Py_DECREF(formats);
		Py_DECREF(module);
		return NULL;
	}
	
	PyModule_AddType(module, &SurfaceType);
	
	return module;
}
