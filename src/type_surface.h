
#define PY_SSIZE_T_CLEAN
#include "gx2/surface.h"
#include <Python.h>

struct SurfaceObject {
	PyObject_HEAD
	GX2Surface surface;
	PyObject *image;
	PyObject *mipmaps;
	PyObject *mip_level_offset;
};

extern PyTypeObject SurfaceType;
