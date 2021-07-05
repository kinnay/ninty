
#define PY_SSIZE_T_CLEAN

#include "dsptool/encoder.h"

#include <Python.h>
#include <cstdint>


struct ADPCMContext {
	int16_t coefs[16];
	uint8_t header;
	int16_t hist1;
	int16_t hist2;
};


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

void encode_pcm8(int8_t *out, const int16_t *in, uint32_t numSamples) {
	for (uint32_t i = 0; i < numSamples; i++) {
		int16_t sample = in[i];
		if (sample < 0x7F80) {
			sample += 0x80;
		}
		
		out[i] = sample >> 8;
	}
}

bool decode_adpcm(
	int16_t *out, const uint8_t *in, uint32_t numSamples, ADPCMContext *ctx
) {
	int nybble;
	int coefIdx;
	int scale = 0;
	int coef1 = 0;
	int coef2 = 0;
	uint8_t byte = 0;
	
	for (uint32_t i = 0; i < numSamples; i++) {
		if (i % 14 == 0) {
			ctx->header = *in++;
			scale = 1 << (ctx->header & 0xF);
			coefIdx = ctx->header >> 4;
			if (coefIdx >= 8) {
				return false;
			}
			coef1 = ctx->coefs[coefIdx * 2];
			coef2 = ctx->coefs[coefIdx * 2 + 1];
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
		int hist = coef1 * ctx->hist1 + coef2 * ctx->hist2;
		int16_t sample = clamp((scaled + hist + 1024) >> 11);
		
		ctx->hist2 = ctx->hist1;
		ctx->hist1 = sample;
		if (out) {
			*out++ = sample;
		}
	}
	return true;
}

void encode_adpcm(
	uint8_t *out, const int16_t *in, uint32_t samples, ADPCMContext *ctx
) {
	dsptool::ADPCMINFO info;
	dsptool::encode(in,  out, &info, samples);
	
	memcpy(ctx->coefs, info.coef, sizeof(info.coef));
	ctx->header = info.pred_scale;
	ctx->hist1 = info.yn1;
	ctx->hist2 = info.yn2;
}

bool parse_adpcm_args(PyObject *args, const uint8_t **in, size_t *inlen, uint32_t *samples, ADPCMContext *ctx) {
	PyObject *coefList;
	if (!PyArg_ParseTuple(args, "y#iO!", in, inlen, samples, &PyList_Type, &coefList)) {
		return false;
	}
	
	size_t size = PyList_Size(coefList);
	if (size != 16) {
		PyErr_SetString(PyExc_ValueError, "len(coefs) must be 16");
		return false;
	}
	
	memset(ctx, 0, sizeof(*ctx));
	for (size_t i = 0; i < 16; i++) {
		PyObject *item = PyList_GetItem(coefList, i);
		if (!PyLong_Check(item)) {
			PyErr_SetString(PyExc_TypeError, "coefs must contain only integers");
			return false;
		}
		
		long value = PyLong_AsLong(item);
		if (PyErr_Occurred()) return false;
		
		if (value < -0x8000 || value > 0x7FFF) {
			PyErr_SetString(PyExc_OverflowError, "coefs must contain 16-bit signed integers");
			return false;
		}
		
		ctx->coefs[i] = value;
	}
	
	size_t bytesNeeded = *samples / 14 * 8;
	if (*samples % 14) {
		bytesNeeded += (*samples % 14 + 1) / 2 + 1;
	}
	
	if (bytesNeeded > *inlen) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow");
		return false;
	}
	
	return true;
}

PyObject *Audio_interleave(PyObject *self, PyObject *args) {
	if (!PyList_Check(args)) {
		PyErr_SetString(PyExc_TypeError, "channels must be a list object");
		return NULL;
	}
	
	size_t count = PyList_Size(args);
	if (!count) {
		return PyBytes_FromString("");
	}
	
	if (count >= 0x10000) {
		PyErr_SetString(PyExc_ValueError, "too many channels");
		return NULL;
	}
	
	ssize_t size;
	for (size_t i = 0; i < count; i++) {
		PyObject *channel = PyList_GetItem(args, i);
		if (!PyBytes_Check(channel)) {
			PyErr_SetString(PyExc_TypeError, "channel must be a bytes object");
			return NULL;
		}
		
		if (i == 0) {
			size = PyBytes_Size(channel);
			if (size % 2) {
				PyErr_SetString(PyExc_ValueError, "channel must contain an even number of bytes");
				return NULL;
			}
			
			if (size > 0x8000000) {
				PyErr_SetString(PyExc_OverflowError, "stream is too large");
				return NULL;
			}
		}
		else {
			if (PyBytes_Size(channel) != size) {
				PyErr_SetString(PyExc_ValueError, "every channel must contain the same number of bytes");
				return NULL;
			}
		}
	}
	
	const int16_t **channels = (const int16_t **)malloc(count * sizeof(int16_t *));
	if (!channels) {
		return PyErr_NoMemory();
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, count * size);
	if (!bytes) {
		free(channels);
		return NULL;
	}
	
	for (size_t i = 0; i < count; i++) {
		PyObject *channel = PyList_GetItem(args, i);
		channels[i] = (const int16_t *)PyBytes_AsString(channel);
	}
	
	int16_t *out = (int16_t *)PyBytes_AsString(bytes);
	for (ssize_t i = 0; i < size / 2; i++) {
		for (size_t j = 0; j < count; j++) {
			out[i * count + j] = channels[j][i];
		}
	}
	
	free(channels);
	return bytes;
}

PyObject *Audio_deinterleave(PyObject *self, PyObject *args) {
	const int16_t *in;
	size_t inlen;
	int channels;
	
	if (!PyArg_ParseTuple(args, "y#i", &in, &inlen, &channels)) {
		return NULL;
	}
	
	if (channels <= 0 || channels >= 65536) {
		PyErr_SetString(PyExc_ValueError, "invalid number of channels");
		return NULL;
	}
	
	if (inlen % (channels * 2)) {
		PyErr_SetString(PyExc_ValueError, "number of samples must be divisible by number of channels");
		return NULL;
	}
	
	PyObject *list = PyList_New(channels);
	if (!list) {
		return NULL;
	}
	
	for (int chan = 0; chan < channels; chan++) {
		PyObject *bytes = PyBytes_FromStringAndSize(NULL, inlen / channels);
		if (!bytes) {
			Py_DECREF(list);
			return NULL;
		}
		
		int16_t *samples = (int16_t *)PyBytes_AsString(bytes);
		for (size_t samp = 0; samp < inlen / channels / 2; samp++) {
			samples[samp] = in[samp * channels + chan];
		}
		
		PyList_SetItem(list, chan, bytes);
	}
	
	return list;
}

PyObject *Audio_decode_pcm8(PyObject *self, PyObject *args) {
	const int8_t *in;
	size_t inlen;
	
	if (!PyArg_ParseTuple(args, "y#", &in, &inlen)) {
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, inlen * 2);
	if (!bytes) return NULL;
	
	int16_t *out = (int16_t *)PyBytes_AsString(bytes);
	decode_pcm8(out, in, inlen);
	
	return bytes;
}

PyObject *Audio_decode_adpcm(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t samples;
	ADPCMContext ctx;
	
	if (!parse_adpcm_args(args, &in, &inlen, &samples, &ctx)) {
		return NULL;
	}
	
	if (samples > 0x4000000) {
		PyErr_SetString(PyExc_OverflowError, "stream is too large");
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, samples * 2);
	if (!bytes) return NULL;
	
	int16_t *out = (int16_t *)PyBytes_AsString(bytes);
	
	bool result = decode_adpcm(out, in, samples, &ctx);
	
	if (!result) {
		Py_DECREF(bytes);
		PyErr_SetString(PyExc_OverflowError, "buffer overflow (coefs)");
		return NULL;
	}
	
	return bytes;
}

PyObject *Audio_encode_pcm8(PyObject *self, PyObject *args) {
	const int16_t *in;
	size_t inlen;
	
	if (!PyArg_ParseTuple(args, "y#", &in, &inlen)) {
		return NULL;
	}
	
	if (inlen % 2) {
		PyErr_SetString(PyExc_ValueError, "buffer must contain an even number of bytes");
		return NULL;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, inlen / 2);
	if (!bytes) return NULL;
	
	int8_t *out = (int8_t *)PyBytes_AsString(bytes);
	encode_pcm8(out, in, inlen / 2);
	
	return bytes;
}

PyObject *Audio_encode_adpcm(PyObject *self, PyObject *args) {
	const int16_t *in;
	size_t inlen;
	
	if (!PyArg_ParseTuple(args, "y#", &in, &inlen)) {
		return NULL;
	}
	
	if (inlen % 2) {
		PyErr_SetString(PyExc_ValueError, "buffer must contain an even number of bytes");
		return NULL;
	}
	
	size_t samples = inlen / 2;
	size_t bytesNeeded = samples / 14 * 8;
	if (samples % 14) {
		bytesNeeded += (samples % 14 + 1) / 2 + 1;
	}
	
	PyObject *bytes = PyBytes_FromStringAndSize(NULL, bytesNeeded);
	if (!bytes) return NULL;
	
	uint8_t *out = (uint8_t *)PyBytes_AsString(bytes);
	
	ADPCMContext ctx;
	encode_adpcm(out, in, samples, &ctx);
	
	PyObject *list = PyList_New(16);
	if (!list) {
		Py_DECREF(bytes);
		return NULL;
	}
	
	for (size_t i = 0; i < 16; i++) {
		PyObject *item = Py_BuildValue("h", ctx.coefs[i]);
		if (!item) {
			Py_DECREF(bytes);
			Py_DECREF(list);
			return NULL;
		}
		
		PyList_SetItem(list, i, item);
	}
	
	PyObject *result = Py_BuildValue("OO", bytes, list);
	Py_DECREF(bytes);
	Py_DECREF(list);
	
	return result;
}

PyObject *Audio_get_adpcm_context(PyObject *self, PyObject *args) {
	const uint8_t *in;
	size_t inlen;
	uint32_t samples;
	ADPCMContext ctx;
	
	if (!parse_adpcm_args(args, &in, &inlen, &samples, &ctx)) {
		return NULL;
	}
	
	bool result = decode_adpcm(NULL, in, samples, &ctx);
	if (!result) {
		PyErr_SetString(PyExc_OverflowError, "buffer overflow (coefs)");
		return NULL;
	}
	
	return Py_BuildValue("Bhh", ctx.header, ctx.hist1, ctx.hist2);
}


PyMethodDef AudioMethods[] = {
	{"interleave", Audio_interleave, METH_O, NULL},
	{"deinterleave", Audio_deinterleave, METH_VARARGS, NULL},
	{"decode_pcm8", Audio_decode_pcm8, METH_VARARGS, NULL},
	{"encode_pcm8", Audio_encode_pcm8, METH_VARARGS, NULL},
	{"decode_adpcm", Audio_decode_adpcm, METH_VARARGS, NULL},
	{"encode_adpcm", Audio_encode_adpcm, METH_VARARGS, NULL},
	{"get_adpcm_context", Audio_get_adpcm_context, METH_VARARGS, NULL},
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
