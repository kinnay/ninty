
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <cstring>

enum class LZSSError {
	OK,
	WrongSize,
	InvalidType,
	BufferOverflow
};

template <typename T, int M>
LZSSError lzss_decompress_internal(const uint8_t *inbase, size_t insize, uint8_t *outbase, size_t outsize) {
	const uint8_t *in = inbase;
	uint8_t *out = outbase;
	
	int bits = 0;
	uint8_t flags;
	while (insize > 0) {
		if (bits == 0) {
			flags = *in++;
			insize -= 1;
			bits = 8;
		}
		
		if (flags & 0x80) {
			if (insize < 2) {
				return LZSSError::BufferOverflow;
			}
			uint16_t info = (in[0] << 8) | in[1];
			insize -= 2;
			in += 2;
			
			int offset = (info & 0xFFF) * sizeof(T);
			size_t length = ((info >> 12) + M) * sizeof(T);
			
			if (offset > out - outbase || length > outsize) {
				return LZSSError::BufferOverflow;
			}
			
			for (size_t i = 0; i < length; i++) {
				out[i] = out[i - offset];
			}
			out += length;
			outsize -= length;
		}
		else {
			if (outsize < sizeof(T) || insize < sizeof(T)) {
				return LZSSError::BufferOverflow;
			}
			*(T *)out = *(T *)in;
			in += sizeof(T);
			out += sizeof(T);
			insize -= sizeof(T);
			outsize -= sizeof(T);
		}
		
		flags <<= 1;
		bits--;
	}
	
	return LZSSError::OK;
}

LZSSError lzss_decompress(const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen) {
	if (inlen < 4) {
		return LZSSError::BufferOverflow;
	}
	
	int type = in[0];
	
	if (type == 0) {
		if (inlen - 4 < outlen) {
			return LZSSError::WrongSize;
		}
		else if (inlen - 4 > outlen) {
			return LZSSError::BufferOverflow;
		}
		memcpy(out, in + 4, inlen - 4);
		return LZSSError::OK;
	}
	
	else if (type == 1) {
		return lzss_decompress_internal<uint8_t, 3>(in + 4, inlen - 4, out, outlen);
	}
	else if (type == 2) {
		return lzss_decompress_internal<uint16_t, 2>(in + 4, inlen - 4, out, outlen);
	}
	else if (type == 3) {
		return lzss_decompress_internal<uint32_t, 1>(in + 4, inlen - 4, out, outlen);
	}
	
	return LZSSError::InvalidType;
}


void LZSS_set_error(LZSSError error) {
	if (error == LZSSError::BufferOverflow) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow");
	}
	else if (error == LZSSError::WrongSize) {
		PyErr_SetString(PyExc_ValueError, "decompressed size is wrong");
	}
	else if (error == LZSSError::InvalidType) {
		PyErr_SetString(PyExc_ValueError, "invalid type value in header");
	}
}

PyObject *LZSS_decompress(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t outlen;
	if (!PyArg_ParseTuple(args, "y#I", &in, &inlen, &outlen)) {
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, outlen);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	LZSSError error = lzss_decompress(in, inlen, out, outlen);
	if (error != LZSSError::OK) {
		Py_DECREF(bytes);
		LZSS_set_error(error);
		return NULL;
	}
	
	return bytes;
}

PyMethodDef LZSSMethods[] = {
	{"decompress", LZSS_decompress, METH_VARARGS, NULL},
	NULL
};

PyModuleDef LZSSModule = {
	PyModuleDef_HEAD_INIT,
	"lzss",
	"LZSS compression methods",
	-1,
	
	LZSSMethods
};

PyMODINIT_FUNC PyInit_lzss() {
	return PyModule_Create(&LZSSModule);
}
