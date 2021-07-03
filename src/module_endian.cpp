
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <cstring>

enum class EndianError {
	OK,
	InvalidSize,
	InvalidParameters,
	SizeNotAligned
};

template <typename T>
T swap_value(T value);

template <>
uint16_t swap_value<uint16_t>(uint16_t value) {
	return (value >> 8) | (value << 8);
}

template <>
uint32_t swap_value<uint32_t>(uint32_t value) {
	uint32_t result = 0;
	result |= value >> 24;
	result |= (value >> 8) & 0xFF00;
	result |= (value << 8) & 0xFF0000;
	result |= value << 24;
	return result;
}

template <typename T>
EndianError swap_array_tmpl(
	uint8_t *data, size_t datasize, size_t size,
	size_t offset, size_t count, size_t stride
) {
	if (datasize % stride) {
		return EndianError::SizeNotAligned;
	}
	if (offset > stride || (stride - offset) / size < count) {
		return EndianError::InvalidParameters;
	}
	
	for (size_t offs = offset; offs < datasize; offs += stride) {
		for (size_t i = 0; i < count; i++) {
			uint8_t *ptr = &data[offs+i*size];
			*(T *)ptr = swap_value<T>(*(T *)ptr);
		}
	}
	return EndianError::OK;
}

EndianError swap_array(
	const uint8_t *in, size_t insize, uint8_t *out, size_t size,
	size_t offset, size_t count, size_t stride
) {
	memcpy(out, in, insize);
	if (size == 1) {
		return EndianError::OK;
	}
	if (size == 2) {
		return swap_array_tmpl<uint16_t>(out, insize, size, offset, count, stride);
	}
	if (size == 4) {
		return swap_array_tmpl<uint32_t>(out, insize, size, offset, count, stride);
	}
	return EndianError::InvalidSize;
}


void Endian_set_error(EndianError error) {
	if (error == EndianError::InvalidSize) {
		PyErr_SetString(PyExc_ValueError, "element size must be 1, 2 or 4");
	}
	else if (error == EndianError::InvalidParameters) {
		PyErr_SetString(PyExc_ValueError, "invalid parameters");
	}
	else if (error == EndianError::SizeNotAligned) {
		PyErr_SetString(PyExc_ValueError, "buffer size must be a multiple of array stride");
	}
}

PyObject *Endian_swap_array(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t size;
	uint32_t offset = 0;
	uint32_t count = 1;
	uint32_t stride;
	
	size_t nargs = PyTuple_Size(args);
	if (nargs == 2) {
		if (!PyArg_ParseTuple(args, "y#I", &in, &inlen, &size)) {
			return NULL;
		}
		stride = size;
	}
	else if (nargs == 4) {
		if (!PyArg_ParseTuple(args, "y#III", &in, &inlen, &size, &offset, &stride)) {
			return NULL;
		}
	}
	else if (nargs == 5) {
		if (!PyArg_ParseTuple(args, "y#IIII", &in, &inlen, &size, &offset, &count, &stride)) {
			return NULL;
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError, "endian.swap_array takes 2, 4 or 5 arguments");
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, inlen);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	EndianError error = swap_array(in, inlen, out, size, offset, count, stride);
	if (error != EndianError::OK) {
		Py_DECREF(bytes);
		Endian_set_error(error);
		return NULL;
	}
	
	return bytes;
}

PyMethodDef EndianMethods[] = {
	{"swap_array", Endian_swap_array, METH_VARARGS, NULL},
	NULL
};

PyModuleDef EndianModule = {
	PyModuleDef_HEAD_INIT,
	"endian",
	"Fast byte swapping methods",
	-1,
	
	EndianMethods
};

PyMODINIT_FUNC PyInit_endian() {
	return PyModule_Create(&EndianModule);
}
