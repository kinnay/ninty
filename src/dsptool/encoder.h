
#pragma once

#include <cstdint>

namespace dsptool {

#define BYTES_PER_FRAME 8
#define SAMPLES_PER_FRAME 14
#define NIBBLES_PER_FRAME 16
	
struct ADPCMINFO {
	int16_t coef[16];
	uint16_t gain;
	uint16_t pred_scale;
	int16_t yn1;
	int16_t yn2;

	uint16_t loop_pred_scale;
	int16_t loop_yn1;
	int16_t loop_yn2;
};

void encode(const int16_t *src, uint8_t *dst, ADPCMINFO *cxt, uint32_t samples);
void getLoopContext(uint8_t *src, ADPCMINFO *cxt, uint32_t samples);

void correlateCoefs(const int16_t *src, uint32_t samples, int16_t *coefsOut);

uint32_t getBytesForAdpcmSamples(uint32_t samples);

}
