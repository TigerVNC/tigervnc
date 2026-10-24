/* Spa */
/* SPDX-FileCopyrightText: Copyright © 2018 Wim Taymans */
/* SPDX-License-Identifier: MIT */

/* Adapted from spa/plugins/audioconvert/resample.h from PipeWire */

#ifndef RESAMPLE_H
#define RESAMPLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RESAMPLE_DEFAULT_QUALITY
#define RESAMPLE_DEFAULT_QUALITY	4
#endif

#define RESAMPLE_WINDOW_DEFAULT		RESAMPLE_WINDOW_EXP
#define RESAMPLE_MAX_PARAMS		16

struct resample_config {
#define RESAMPLE_WINDOW_EXP		0
#define RESAMPLE_WINDOW_BLACKMAN	1
	uint32_t window;

	double cutoff;
	uint32_t n_taps;

#define RESAMPLE_PARAM_EXP_A			0
#define RESAMPLE_PARAM_BLACKMAN_ALPHA		0
	double params[RESAMPLE_MAX_PARAMS];
};

struct resample {
#define RESAMPLE_OPTION_PREFILL		(1<<0)
	uint32_t options;

	uint32_t channels;
	uint32_t i_rate;
	uint32_t o_rate;
	int quality;

	struct resample_config config;	/* set to all 0 for defaults */

	void *data;
};

int resample_init(struct resample *r);
void resample_free(struct resample *r);
void resample_update_rate(struct resample *r, double rate);
uint32_t resample_in_len(struct resample *r, uint32_t out_len);
uint32_t resample_out_len(struct resample *r, uint32_t in_len);
void resample_process(struct resample *r,
		const void *src[], uint32_t *in_len,
		void *dst[], uint32_t *out_len);
void resample_reset(struct resample *r);
uint32_t resample_delay(struct resample *r);

/** Fractional part of delay (in input samples) */
float resample_phase(struct resample *r);

#ifdef __cplusplus
}
#endif

#endif /* RESAMPLE_H */
