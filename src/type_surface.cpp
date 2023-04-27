
#include "type_surface.h"

#include "gx2/enum.h"
#include "gx2/codecs.h"
#include "gx2/facade.h"

#include "structmember.h"

bool prepare_surface(SurfaceObject *self) {
	GX2CalcSurfaceSizeAndAlignment(&self->surface);
	
	if (!self->image || !PyBytes_Check(self->image)) {
		PyErr_SetString(PyExc_TypeError, "image must be a bytes object");
		return false;
	}
	
	if (PyBytes_Size(self->image) < self->surface.image_size) {
		PyErr_SetString(PyExc_TypeError, "image buffer is too small");
		return false;
	}
	
	if (self->surface.mip_levels > 1) {
		if (!self->mipmaps || !PyBytes_Check(self->mipmaps)) {
			PyErr_SetString(PyExc_TypeError, "mipmaps must be a bytes object");
			return false;
		}
		
		if (PyBytes_Size(self->mipmaps) < self->surface.mipmap_size) {
			PyErr_SetString(PyExc_TypeError, "mipmaps buffer is too small");
			return false;
		}
	}
	
	self->surface.image = (uint8_t *)PyBytes_AS_STRING(self->image);
	self->surface.mipmaps = (uint8_t *)PyBytes_AS_STRING(self->mipmaps);
	return true;
}

bool update_surface(SurfaceObject *self, GX2Surface *surface) {
	self->surface = *surface;
	
	self->image = PyBytes_FromStringAndSize((char *)surface->image, surface->image_size);
	free(surface->image);
	
	if (!self->image) return false;
	
	self->mipmaps = PyBytes_FromStringAndSize((char *)surface->mipmaps, surface->mipmap_size);
	if (!self->mipmaps) return false;
	
	self->mip_level_offset = PyList_New(13);
	for (size_t i = 0; i < 13; i++) {
		PyObject *value = PyLong_FromLong(surface->mip_level_offset[i]);
		if (!value) {
			Py_CLEAR(self->mip_level_offset);
			return false;
		}
		
		PyList_SET_ITEM(self->mip_level_offset, i, value);
	}
	return true;
}

int Surface_init(SurfaceObject *self, PyObject *args, PyObject *kwargs) {
	GX2Surface *surface = &self->surface;
	surface->dim = GX2_SURFACE_DIM_TEXTURE_2D;
	surface->width = 0;
	surface->height = 0;
	surface->depth = 0;
	surface->mip_levels = 0;
	surface->format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
	surface->aa = GX2_AA_MODE_1X;
	surface->use = GX2_SURFACE_USE_TEXTURE;
	surface->image_size = 0;
	surface->mipmap_size = 0;
	surface->tile_mode = GX2_TILE_MODE_LINEAR_SPECIAL;
	surface->swizzle = 0;
	surface->alignment = 0;
	surface->pitch = 0;
	
	self->image = PyBytes_FromString("");
	if (!self->image) return -1;
	
	self->mipmaps = PyBytes_FromString("");
	if (!self->mipmaps) return -1;
	
	self->mip_level_offset = PyList_New(13);
	for (size_t i = 0; i < 13; i++) {
		PyObject *value = PyLong_FromLong(0);
		if (!value) {
			Py_CLEAR(self->mip_level_offset);
			return -1;
		}
		
		PyList_SET_ITEM(self->mip_level_offset, i, value);
	}
	return 0;
}

void Surface_dealloc(SurfaceObject *self) {
	Py_XDECREF(self->image);
	Py_XDECREF(self->mipmaps);
	Py_XDECREF(self->mip_level_offset);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *Surface_calc_size_and_alignment(SurfaceObject *self, PyObject *args) {
	GX2CalcSurfaceSizeAndAlignment(&self->surface);
	Py_RETURN_NONE;
}

PyObject *Surface_deswizzle(SurfaceObject *self, PyObject *args) {
	if (self->surface.tile_mode != GX2_TILE_MODE_LINEAR_SPECIAL) {
		if (!prepare_surface(self)) return NULL;
		
		GX2Surface output;
		if (!gx2::convert_tilemode(&self->surface, &output, GX2_TILE_MODE_LINEAR_SPECIAL, 0)) {
			return PyErr_NoMemory();
		}
		
		if (!update_surface(self, &output)) return NULL;
	}
	Py_RETURN_NONE;
}

PyObject *Surface_swizzle(SurfaceObject *self, PyObject *args) {
	GX2TileMode tilemode;
	uint8_t swizzle;
	
	if (!PyArg_ParseTuple(args, "Ib", &tilemode, swizzle)) {
		return NULL;
	}
	
	if (!prepare_surface(self)) return NULL;
		
	GX2Surface output;
	if (!gx2::convert_tilemode(&self->surface, &output, tilemode, swizzle)) {
		return PyErr_NoMemory();
	}
	
	if (!update_surface(self, &output)) return NULL;
	
	Py_RETURN_NONE;
}

PyObject *Surface_decode(SurfaceObject *self, PyObject *args) {
	if (!prepare_surface(self)) return NULL;
	
	if (self->surface.tile_mode != GX2_TILE_MODE_LINEAR_SPECIAL) {
		PyErr_SetString(PyExc_ValueError, "surface must be deswizzled before conversion");
		return NULL;
	}
	
	if (!gx2::is_format_supported(self->surface.format)) {
		PyErr_SetString(PyExc_ValueError, "surface format not supported");
		return NULL;
	}
	
	GX2Surface output;
	if (!gx2::decode(&self->surface, &output)) {
		return PyErr_NoMemory();
	}
	
	if (!update_surface(self, &output)) return NULL;
	
	Py_RETURN_NONE;
}

PyMemberDef Surface_members[] = {
	{"dim", T_UINT, offsetof(SurfaceObject, surface.dim)},
	{"width", T_UINT, offsetof(SurfaceObject, surface.width)},
	{"height", T_UINT, offsetof(SurfaceObject, surface.height)},
	{"depth", T_UINT, offsetof(SurfaceObject, surface.depth)},
	{"mip_levels", T_UINT, offsetof(SurfaceObject, surface.mip_levels)},
	{"format", T_UINT, offsetof(SurfaceObject, surface.format)},
	{"aa", T_UINT, offsetof(SurfaceObject, surface.aa)},
	{"use", T_UINT, offsetof(SurfaceObject, surface.use)},
	{"image_size", T_UINT, offsetof(SurfaceObject, surface.image_size)},
	{"image", T_OBJECT_EX, offsetof(SurfaceObject, image)},
	{"mipmap_size", T_UINT, offsetof(SurfaceObject, surface.mipmap_size)},
	{"mipmaps", T_OBJECT_EX, offsetof(SurfaceObject, mipmaps)},
	{"tile_mode", T_UINT, offsetof(SurfaceObject, surface.tile_mode)},
	{"swizzle", T_UINT, offsetof(SurfaceObject, surface.swizzle)},
	{"alignment", T_UINT, offsetof(SurfaceObject, surface.alignment)},
	{"pitch", T_UINT, offsetof(SurfaceObject, surface.pitch)},
	{"mip_level_offset", T_OBJECT_EX, offsetof(SurfaceObject, mip_level_offset)},
	{NULL}
};

PyMethodDef Surface_methods[] = {
	{"calc_size_and_alignment", (PyCFunction)Surface_calc_size_and_alignment, METH_NOARGS},
	{"deswizzle", (PyCFunction)Surface_deswizzle, METH_NOARGS},
	{"swizzle", (PyCFunction)Surface_swizzle, METH_VARARGS},
	{"decode", (PyCFunction)Surface_decode, METH_NOARGS},
	{NULL}
};

PyTypeObject SurfaceType = []() -> PyTypeObject {
	PyTypeObject type = {PyVarObject_HEAD_INIT(NULL, 0)};
	type.tp_name = "Surface";
	type.tp_doc = "A GX2Surface object";
	type.tp_basicsize = sizeof(SurfaceObject);
	type.tp_flags = Py_TPFLAGS_DEFAULT;
	type.tp_dealloc = (destructor)Surface_dealloc;
	type.tp_new = PyType_GenericNew;
	type.tp_init = (initproc)Surface_init;
	type.tp_members = Surface_members;
	type.tp_methods = Surface_methods;
	return type;
}();
