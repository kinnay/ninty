
#define PY_SSIZE_T_CLEAN
#include "gx2/surface.h"
#include <Python.h>

struct SurfaceObject {
	PyObject_HEAD
	GX2Surface surface;
	PyObject *image;
	PyObject *mipmaps;
	PyObject *mip_level_offset;
	/*
	GX2SurfaceDim dim;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mip_levels;
	GX2SurfaceFormat format;
	GX2AAMode aa;
	GX2SurfaceUse use;
	uint32_t image_size;
	PyObject *image;
	uint32_t mipmap_size;
	PyObject *mipmaps;
	GX2TileMode tile_mode;
	uint32_t swizzle;
	uint32_t alignment;
	uint32_t pitch;
	PyObject *mip_level_offset;*/
};

extern PyTypeObject SurfaceType;
