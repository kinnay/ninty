
#include <Python.h>
#include <cstdint>

int16_t clamp(int val) {
	if (val < -32768) return -32768;
	if (val > 32767) return 32767;
	return val;
}

bool decode_adpcm(
	int16_t *out, const uint8_t *in, uint32_t numSamples,
	int16_t *coefs, uint8_t header, int16_t hist1, int16_t hist2
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


PyObject *ADPCM_decode(PyObject *self, PyObject *args) {
	const uint8_t *in;
	uint32_t inlen;
	uint32_t numSamples;
	const uint8_t *coefBuf;
	uint32_t coefLen;
	uint8_t initialHeader;
	uint16_t initialHist1;
	uint16_t initialHist2;
	
	if (!PyArg_ParseTuple(args, "y#iy#bhh",
		&in, &inlen, &numSamples, &coefBuf, &coefLen,
		&initialHeader, &initialHist1, &initialHist2
	)) {
		return NULL;
	}
	
	if (coefLen != 32) {
		PyErr_SetString(PyExc_ValueError, "len(coefs) must be 32");
		return NULL;
	}
	
	uint32_t bytesNeeded = numSamples / 14 * 8;
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
	
	int16_t coefs[16];
	for (int i = 0; i < 16; i++) {
		coefs[i] = (coefBuf[i * 2] << 8) | coefBuf[i * 2 + 1];
	}
	
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

PyMethodDef ADPCMMethods[] = {
	{"decode", ADPCM_decode, METH_VARARGS, NULL},
	NULL
};

PyModuleDef ADPCMModule = {
	PyModuleDef_HEAD_INIT,
	"adpcm",
	"ADPCM decoder",
	-1,

	ADPCMMethods
};

PyMODINIT_FUNC PyInit_adpcm() {
	return PyModule_Create(&ADPCMModule);
}
