
#include "gx2/compression.h"

namespace gx2 {

void bc_interpolate(uint8_t *colors, int count) {
	for (int i = 1; i < count; i++) {
		colors[i+1] = (colors[0] * (count - i) + colors[1] * i) / count;
	}
}

void bc1_interpolate(bool mode, uint8_t *colors) {
	if (mode) {
		bc_interpolate(colors, 3);
	}
	else {
		bc_interpolate(colors, 2);
		colors[3] = 0;
	}
}

void bc3_interpolate(uint8_t *colors) {
	if (colors[0] > colors[1]) {
		bc_interpolate(colors, 7);
	}
	else {
		bc_interpolate(colors, 5);
		colors[6] = 0;
		colors[7] = 0xFF;
	}
}
	
void bc_decode_565(uint16_t color, uint8_t *red, uint8_t *green, uint8_t *blue) {
	red[0] = (color >> 11) << 3;
	green[0] = (color >> 3) & 0xFC;
	blue[0] = (color & 0x1F) << 3;
}

void decompress_bc1(uint8_t *out, const uint8_t *in) {
	uint16_t color0 = in[0] | (in[1] << 8);
	uint16_t color1 = in[2] | (in[3] << 8);
	in += 4;
	
	uint8_t red[4], green[4], blue[4];
	bc_decode_565(color0, red, green, blue);
	bc_decode_565(color1, red+1, green+1, blue+1);
	
	uint8_t alpha[4];
	alpha[0] = 0xFF;
	alpha[1] = 0xFF;
	
	bc1_interpolate(color0 > color1, red);
	bc1_interpolate(color0 > color1, green);
	bc1_interpolate(color0 > color1, blue);
	bc1_interpolate(color0 > color1, alpha);
	
	for (int y = 0; y < 4; y++) {
		uint8_t pixels = in[0];
		for (int x = 0; x < 4; x++) {
			*out++ = red[pixels & 3];
			*out++ = green[pixels & 3];
			*out++ = blue[pixels & 3];
			*out++ = alpha[pixels & 3];
			pixels >>= 2;
		}
		in++;
	}
}

void decompress_bc3(uint8_t *out, const uint8_t *in) {
	uint8_t alpha[8];
	alpha[0] = in[0];
	alpha[1] = in[1];
	
	bc3_interpolate(alpha);
	
	uint16_t color0 = in[8] | (in[9] << 8);
	uint16_t color1 = in[10] | (in[11] << 8);
	
	uint8_t red[4], green[4], blue[4];
	bc_decode_565(color0, red, green, blue);
	bc_decode_565(color1, red+1, green+1, blue+1);
	
	bc1_interpolate(true, red);
	bc1_interpolate(true, green);
	bc1_interpolate(true, blue);
	
	uint32_t alphabits = in[2] | (in[3] << 8) | (in[4] << 16);
	uint32_t alphabits2 = in[5] | (in[6] << 8) | (in[7] << 16);
	
	for (int y = 0; y < 4; y++) {
		if (y == 2) {
			alphabits = alphabits2;
		}
		
		uint8_t colorbits = in[y + 12];
		for (int x = 0; x < 4; x++) {
			*out++ = red[colorbits & 3];
			*out++ = green[colorbits & 3];
			*out++ = blue[colorbits & 3];
			*out++ = alpha[alphabits & 7];
			alphabits >>= 3;
			colorbits >>= 2;
		}
	}
}

void decompress_bc5(uint8_t *out, const uint8_t *in) {
	uint8_t red[8], green[8];
	red[0] = in[0];
	red[1] = in[1];
	green[0] = in[8];
	green[1] = in[9];
	
	bc3_interpolate(red);
	bc3_interpolate(green);
	
	uint32_t redbits = in[2] | (in[3] << 8) | (in[4] << 16);
	uint32_t redbits2 = in[5] | (in[6] << 8) | (in[7] << 16);
	uint32_t greenbits = in[10] | (in[11] << 8) | (in[12] << 16);
	uint32_t greenbits2 = in[13] | (in[14] << 8) | (in[15] << 16);
	
	for (int y = 0; y < 4; y++) {
		if (y == 2) {
			redbits = redbits2;
			greenbits = greenbits2;
		}
		
		for (int x = 0; x < 4; x++) {
			*out++ = red[redbits & 7];
			*out++ = green[greenbits & 7];
			*out++ = 0;
			*out++ = 0xFF;
			redbits >>= 3;
			greenbits >>= 3;
		}
	}
}

}
