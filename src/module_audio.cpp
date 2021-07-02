
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>

int16_t clamp(int val) {
	if (val < -32768) return -32768;
	if (val > 32767) return 32767;
	return val;
}

void decode_pcm8(int16_t *out, const int8_t *in, uint32_t numSamples) {
	for (uint32_t i = 0; i < numSamples; i++) {
		out[i] = in[i] << 8;
	}
}

bool decode_adpcm(
	int16_t *out, const uint8_t *in, uint32_t numSamples,
	const int16_t *coefs, uint8_t header, int16_t hist1, int16_t hist2
) {
	int scale, coefIdx, coef1, coef2, nybble;
	uint8_t byte;
	
	for (uint32_t i = 0; i < numSamples; i++) {
		if (i % 14 == 0) {
			header = *in++;
			scale = 1 << (header & 0xF);
			coefIdx = header >> 4;
			if (coefIdx >= 8) {
				return false;
			}
			coef1 = coefs[coefIdx * 2];
			coef2 = coefs[coefIdx * 2 + 1];
		}
		
		if (i % 2) {
			nybble = byte & 0xF;
		}
		else {
			byte = *in++;
			nybble = byte >> 4;
		}
		
		if (nybble >= 8) {
			nybble -= 16;
		}
		
		int scaled = (nybble * scale) << 11;
		int hist = coef1 * hist1 + coef2 * hist2;
		int16_t sample = clamp((scaled + hist + 1024) >> 11);
		
		hist2 = hist1;
		hist1 = sample;
		*out++ = sample;
	}
	return true;
}

PyObject *Audio_decode_pcm8(PyObject *self, PyObject *args) {
	const int8_t *in;
	size_t inlen;
	uint32_t numSamples;
	
	if (!PyArg_ParseTuple(args, "y#i", &in, &inlen, &numSamples)) {
		return NULL;
	}
	
	if (inlen < numSamples) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow");
		return NULL;
	}
	
	if (numSamples > 0x4000000) {
		PyErr_SetString(PyExc_OverflowError, "stream is too large");
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, numSamples * 2);
	if (!bytes) return NULL;
	
	int16_t *out = (int16_t *)PyBytes_AsString(bytes);
	decode_pcm8(out, in, numSamples);
	
	return bytes;
}

PyObject *Audio_decode_adpcm(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t numSamples;
	PyObject *coefList;
	uint8_t initialHeader;
	int16_t initialHist1;
	int16_t initialHist2;
	
	if (!PyArg_ParseTuple(args, "y#iO!bhh",
		&in, &inlen, &numSamples, &PyList_Type, &coefList,
		&initialHeader, &initialHist1, &initialHist2
	)) {
		return NULL;
	}
	
	size_t size = PyList_Size(coefList);
	if (size != 16) {
		PyErr_SetString(PyExc_ValueError, "len(coefs) must be 16");
		return NULL;
	}
	
	int16_t coefs[16];
	for (size_t i = 0; i < 16; i++) {
		PyObject *item = PyList_GetItem(coefList, i);
		if (!PyLong_Check(item)) {
			PyErr_SetString(PyExc_TypeError, "coefs must contain only integers");
			return NULL;
		}
		
		long value = PyLong_AsLong(item);
		if (PyErr_Occurred()) return NULL;
		
		if (value < -0x8000 || value > 0x7FFF) {
			PyErr_SetString(PyExc_OverflowError, "coefs must contain 16-bit signed integers");
			return NULL;
		}
		
		coefs[i] = value;
	}
	
	size_t bytesNeeded = numSamples / 14 * 8;
	if (numSamples % 14) {
		bytesNeeded += (numSamples % 14 + 1) / 2 + 1;
	}
	
	if (bytesNeeded > inlen) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow");
		return NULL;
	}
	
	if (numSamples > 0x4000000) {
		PyErr_SetString(PyExc_OverflowError, "stream is too large");
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, numSamples * 2);
	if (!bytes) return NULL;
	
	int16_t *out = (int16_t *)PyBytes_AsString(bytes);
	
	bool result = decode_adpcm(
		out, in, numSamples, coefs,
		initialHeader, initialHist1, initialHist2
	);
	
	if (!result) {
		Py_DECREF(bytes);
		PyErr_SetString(PyExc_OverflowError, "buffer overflow (coefs)");
		return NULL;
	}
	
	return bytes;
}


PyMethodDef AudioMethods[] = {
	{"decode_pcm8", Audio_decode_pcm8, METH_VARARGS, NULL},
	{"decode_adpcm", Audio_decode_adpcm, METH_VARARGS, NULL},
	NULL
};

PyModuleDef AudioModule = {
	PyModuleDef_HEAD_INIT,
	"audio",
	"Audio conversion",
	-1,

	AudioMethods
};

PyMODINIT_FUNC PyInit_audio() {
	return PyModule_Create(&AudioModule);
}
