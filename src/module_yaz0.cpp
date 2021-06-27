
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <cstring>

enum YAZ0Error {
	OK,
	InvalidSearchSize,
	FileTooLarge,
	BufferOverflow
};

const uint8_t *findblock(const uint8_t *searchstart, const uint8_t *in, size_t maxsize, size_t *sizeptr) {
	if (maxsize < 3) {
		return NULL;
	}
	
	size_t blocksize = maxsize;
	if (blocksize > 0xFF + 0x12) {
		blocksize = 0xFF + 0x12;
	}
	
	size_t bestsize = 2;
	const uint8_t *bestptr = NULL;
	
	searchstart = (uint8_t *)memchr(searchstart, in[0], in - searchstart);
	while (searchstart) {
		size_t matchsize = 1;
		while (matchsize < blocksize) {
			if (searchstart[matchsize] != in[matchsize]) {
				break;
			}
			matchsize++;
		}
			
		if (matchsize > bestsize) {
			bestptr = searchstart;
			bestsize = matchsize;
			if (bestsize == blocksize) {
				break;
			}
		}
		
		searchstart = (uint8_t *)memchr(searchstart + 1, in[0], in - searchstart - 1);
	}
	
	*sizeptr = bestsize;
	return bestptr;
}

YAZ0Error yaz0_compress(const uint8_t *inbase, size_t inlen, uint8_t *outbase, size_t *outlen, int searchsize) {
	uint8_t *out = outbase;
	const uint8_t *in = inbase;
	const uint8_t *inend = inbase + inlen;
	
	uint8_t *codeptr = out++;
	uint8_t code = 0xFF;
	uint8_t bits = 0;
	while (in < inend) {
		int maxsize = searchsize;
		
		const uint8_t *searchstart = in - searchsize;
		if (in - inbase < maxsize) {
			searchstart = inbase;
		}
		if (inend - in < maxsize) {
			maxsize = inend - in;
		}
		
		size_t bestsize;
		const uint8_t *bestptr = findblock(searchstart, in, maxsize, &bestsize);
		if (bestptr) {
			code &= ~(1 << (7 - bits));
			uint32_t offset = in - bestptr - 1;
			if (bestsize >= 0x12) {
				*out++ = offset >> 8;
				*out++ = offset & 0xFF;
				*out++ = (bestsize - 0x12) & 0xFF;
			}
			else {
				*out++ = (((bestsize - 2) << 4) | (offset >> 8)) & 0xFF;
				*out++ = offset & 0xFF;
			}
			in += bestsize;
		}
		else {
			*out++ = *in++;
		}
		
		if (++bits == 8) {
			*codeptr = code;
			codeptr = out++;
			code = 0xFF;
			bits = 0;
		}
	}
	
	if (bits) {
		*codeptr = code;
	}
	
	*outlen = out - outbase;
	return YAZ0Error::OK;
}

YAZ0Error yaz0_decompress(const uint8_t *inbase, size_t inlen, uint8_t *outbase, size_t outlen) {
	uint8_t *out = outbase;
	uint8_t *outend = outbase + outlen;
	const uint8_t *in = inbase;
	const uint8_t *inend = inbase + inlen;
	uint8_t code = 0;
	int bits = 0;
	while (out < outend && in < inend) {
		if (!bits) {
			code = *in++;
			bits = 8;
		}
		
		if (code & 0x80) {
			if (in >= inend) return YAZ0Error::BufferOverflow;
			*out++ = *in++;
		}
		else {
			if (in > inend - 2) return YAZ0Error::BufferOverflow;
			uint8_t byte1 = *in++;
			uint8_t byte2 = *in++;

			int num = byte1 >> 4;
			int offset = ((byte1 & 0xF) << 8 | byte2) + 1;
			uint8_t *copy = out - offset;
			if (!num) {
				if (in >= inend) return YAZ0Error::BufferOverflow;
				num = *in++ + 0x12;
			}
			else {
				num += 2;
			}
			
			if (copy < outbase || out > outend - num) return YAZ0Error::BufferOverflow;
			while (num--) {
				*out++ = *copy++;
			}
		}
		
		code <<= 1;
		bits--;
	}
	
	return YAZ0Error::OK;
}


void YAZ0_set_error(YAZ0Error error) {
	if (error == YAZ0Error::InvalidSearchSize) {
		PyErr_SetString(PyExc_ValueError, "invalid search size");
	}
	else if (error == YAZ0Error::FileTooLarge) {
		PyErr_SetString(PyExc_OverflowError, "file is too big");
	}
	else if (error == YAZ0Error::BufferOverflow) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow");
	}
}

PyObject *YAZ0_decompress(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t outlen;
	if (!PyArg_ParseTuple(args, "y#I", &in, &inlen, &outlen)) {
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, outlen);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	YAZ0Error error = yaz0_decompress(in, inlen, out, outlen);
	if (error != YAZ0Error::OK) {
		Py_DECREF(bytes);
		YAZ0_set_error(error);
		return NULL;
	}
	
	return bytes;
}

PyObject *YAZ0_compress(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t searchsize;
	if (!PyArg_ParseTuple(args, "y#I", &in, &inlen, &searchsize)) {
		return NULL;
	}
	
	if (searchsize > 4096) {
		YAZ0_set_error(YAZ0Error::InvalidSearchSize);
		return NULL;
	}
	
	if (inlen > 0x10000000) {
		YAZ0_set_error(YAZ0Error::FileTooLarge);
		return NULL;
	}
	
	size_t outlen = inlen + inlen / 8 + 1;
	uint8_t *out = (uint8_t *)PyMem_RawMalloc(outlen);
	if (!out) {
		return PyErr_NoMemory();
	}
	
	YAZ0Error error = yaz0_compress(in, inlen, out, &outlen, searchsize);
	if (error != YAZ0Error::OK) {
		PyMem_RawFree(out);
		YAZ0_set_error(error);
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize((char *)out, outlen);
	PyMem_RawFree(out);

	return bytes;
}

PyMethodDef YAZ0Methods[] = {
	{"compress", YAZ0_compress, METH_VARARGS, NULL},
	{"decompress", YAZ0_decompress, METH_VARARGS, NULL},
	NULL
};

PyModuleDef YAZ0Module = {
	PyModuleDef_HEAD_INIT,
	"yaz0",
	"Yaz0 compression methods",
	-1,

	YAZ0Methods
};

PyMODINIT_FUNC PyInit_yaz0() {
	return PyModule_Create(&YAZ0Module);
}
