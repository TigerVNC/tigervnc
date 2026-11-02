/* Spa */
/* SPDX-FileCopyrightText: Copyright © 2019 Wim Taymans */
/* SPDX-License-Identifier: MIT */

/* Adapted from spa/plugins/audioconvert/resample-native.c from PipeWire */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "resampler.h"

#define MIN(a,b)		((a) < (b) ? (a) : (b))
#define CLAMP(v,low,high)	((v) < (low) ? (low) : (v) > (high) ? (high) : (v))
#define ROUND_UP_N(num,align)	(((num) + (align) - 1) / (align) * (align))
#define N_ELEMENTS(arr)		(sizeof(arr) / sizeof((arr)[0]))

#define overflow_add(a, b, res)	__builtin_add_overflow(a, b, res)
#define overflow_mul(a, b, res)	__builtin_mul_overflow(a, b, res)

#define FIXP_SHIFT		32
#define FIXP_SCALE		((uint64_t)1 << FIXP_SHIFT)
#define FIXP_MASK		(FIXP_SCALE - 1)
#define UINT32_TO_FIXP(v)	((struct fixp) { (uint64_t)((uint32_t)(v)) << FIXP_SHIFT })
#define FIXP_TO_UINT32(f)	((f).value >> FIXP_SHIFT)
#define FIXP_TO_FLOAT(f)	((f).value / (float)FIXP_SCALE)

struct fixp {
	uint64_t value;
};

struct native_data {
	double rate;
	uint32_t n_taps;
	uint32_t n_phases;
	struct fixp in_rate;
	uint32_t out_rate;
	struct fixp phase;
	float pm;
	uint32_t inc;
	struct fixp frac;
	uint32_t filter_stride;
	uint32_t hist;
	float **history;
	float *filter;
	float *hist_mem;
};

#define MAX_TAPS	(1u<<18)
#define MAX_PHASES	1024u

#define INHERIT_PARAM(c,q,p)	if ((c)->params[p] == 0.0) (c)->params[p] = (q)->params[p];

struct quality {
	uint32_t n_taps;
	double cutoff_up;		/* when upsampling */
	double cutoff_down;		/* for downsampling */
	double params[RESAMPLE_MAX_PARAMS];
};


struct window_info {
	uint32_t window;
	void (*func) (struct resample *r, double *w, double t, uint32_t n_taps);
	uint32_t n_qualities;
	const struct quality *qualities;
	void (*config) (struct resample *r, const struct quality *q);
};

static const struct quality blackman_qualities[] = {
	{ 8, 0.667, 0.58, { 0.16, }},
	{ 16, 0.80, 0.70, { 0.20, }},
	{ 24, 0.857, 0.77, { 0.16, }},
	{ 32, 0.889, 0.82, { 0.16, }},
	{ 48, 0.923, 0.87, { 0.16, }},                  /* default */
	{ 64, 0.941, 0.895, { 0.16, }},
	{ 80, 0.952, 0.910, { 0.16, }},
	{ 96, 0.960, 0.925, { 0.16, }},
	{ 128, 0.970, 0.942, { 0.16, }},
	{ 144, 0.973, 0.950, { 0.16, }},
	{ 160, 0.976, 0.958, { 0.16, }},
	{ 192, 0.980, 0.966, { 0.16, }},
	{ 256, 0.985, 0.975, { 0.16, }},
	{ 896, 0.988, 0.988, { 0.16, }},
	{ 1024, 0.990, 0.990, { 0.16, }},
};

static inline void blackman_window(struct resample *r, double *w, double t, uint32_t n_taps)
{
	double x, alpha = r->config.params[RESAMPLE_PARAM_BLACKMAN_ALPHA];
	uint32_t i, n_taps12 = n_taps/2;
	for (i = 0; i < n_taps12; i++, t += 1.0) {
		x =  2.0 * M_PI * t / n_taps;
		w[i] = (1.0 - alpha) / 2.0 + (1.0 / 2.0) * cos(x) +
			(alpha / 2.0) * cos(2.0 * x);
	}
}
static inline void blackman_config(struct resample *r, const struct quality *q)
{
	INHERIT_PARAM(&r->config, q, RESAMPLE_PARAM_BLACKMAN_ALPHA);
}

static const struct quality exp_qualities[] = {
	{ 8, 0.667, 0.58, { 16.97789, }},
	{ 16, 0.80, 0.70, { 16.97789, }},
	{ 24, 0.857, 0.77, { 16.97789, }},
	{ 32, 0.889, 0.82, { 16.97789, }},
	{ 48, 0.923, 0.87, { 16.97789, }},                  /* default */
	{ 64, 0.941, 0.895, { 16.97789, }},
	{ 80, 0.952, 0.910, { 16.97789, }},
	{ 96, 0.960, 0.925, { 16.97789, }},
	{ 128, 0.970, 0.942, { 16.97789, }},
	{ 144, 0.973, 0.950, { 16.97789, }},
	{ 160, 0.976, 0.958, { 16.97789, }},
	{ 192, 0.980, 0.966, { 16.97789, }},
	{ 256, 0.985, 0.975, { 16.97789, }},
	{ 896, 0.988, 0.988, { 16.97789, }},
	{ 1024, 0.990, 0.990, { 16.97789, }},
};

static inline void exp_window(struct resample *r, double *w, double t, uint32_t n_taps)
{
	double x, A = r->config.params[RESAMPLE_PARAM_EXP_A];
	uint32_t i, n_taps12 = n_taps/2;

	for (i = 0; i < n_taps12; i++, t += 1.0) {
		x =  2.0 * t / n_taps;
		/* doi:10.1109/RME.2008.4595727 with tweak */
		w[i] = (exp(A * sqrt(fmax(0.0, 1.0 - x*x))) - 1) / (exp(A) - 1);
	}
}
static inline void exp_config(struct resample *r, const struct quality *q)
{
	INHERIT_PARAM(&r->config, q, RESAMPLE_PARAM_EXP_A);
}

static const struct window_info window_info[] = {
	[RESAMPLE_WINDOW_EXP] = { RESAMPLE_WINDOW_EXP, exp_window,
		N_ELEMENTS(exp_qualities), exp_qualities, exp_config },
	[RESAMPLE_WINDOW_BLACKMAN] = { RESAMPLE_WINDOW_BLACKMAN, blackman_window,
		N_ELEMENTS(blackman_qualities), blackman_qualities, blackman_config },
};

static inline double sinc(double x, double cutoff)
{
	if (x < 1e-6) return cutoff;
	x *= M_PI;
	return sin(x * cutoff) / x;
}

static int build_filter(struct resample *r, float *taps, uint32_t stride, uint32_t n_taps,
		uint32_t n_phases, double cutoff, double *window)
{
	uint32_t i, j, n_taps12 = n_taps/2;

	for (i = 0; i <= n_phases; i++) {
		double t = (double) i / (double) n_phases;
		window_info[r->config.window].func(r, window, t, n_taps);
		for (j = 0; j < n_taps12; j++, t += 1.0) {
			/* exploit symmetry in filter taps */
			taps[(n_phases - i) * stride + n_taps12 + j] =
				taps[i * stride + (n_taps12 - j - 1)] = (float)
					(sinc(t, cutoff) * window[j]);
		}
	}
	return 0;
}

#define INC(index,phase,n_phases) 						\
	index += inc;								\
	phase += frac;								\
	if (phase >= n_phases) {						\
		phase -= n_phases;						\
		index += 1;							\
	}

static inline void inner_product_ip(float *d, const float * restrict s,
	const float * restrict t0, const float * restrict t1, float x,
	uint32_t n_taps)
{
	float sum[2] = { 0.0f, 0.0f };
	uint32_t i, j, nt2 = n_taps/2;
	for (i = 0, j = n_taps-1; i < nt2; i++, j--) {
		sum[0] += s[i] * t0[i] + s[j] * t0[j];
		sum[1] += s[i] * t1[i] + s[j] * t1[j];
	}
	*d = (sum[1] - sum[0]) * x + sum[0];
}

static void do_resample_inter(struct resample *r,
	const void * restrict src[], uint32_t ioffs, uint32_t *in_len,
	void * restrict dst[], uint32_t ooffs, uint32_t *out_len)
{
	struct native_data *data = r->data;
	uint32_t index, stride = data->filter_stride;
	uint32_t n_taps = data->n_taps;
	uint32_t c, o, olen = *out_len, ilen = *in_len;
	uint32_t inc = data->inc, ch = r->channels;
	uint32_t ph_max = data->n_phases - 1;
	uint64_t frac = data->frac.value, phase = data->phase.value;
	uint64_t denom = UINT32_TO_FIXP(data->out_rate).value;
	float pm = data->pm;

	index = ioffs;
	for (o = ooffs; o < olen && index + n_taps <= ilen; o++) {
		float ph = phase * pm;
		uint32_t offset = MIN((uint32_t)floorf(ph), ph_max);
		float *filter0 = &data->filter[(offset+0) * stride];
		float *filter1 = &data->filter[(offset+1) * stride];
		float pho = ph - offset;
		for (c = 0; c < ch; c++) {
			const float *s = src[c];
			float *d = dst[c];
			inner_product_ip(&d[o], &s[index],
					filter0, filter1, pho, n_taps);
		}
		INC(index, phase, denom);
	}
	*in_len = index;
	*out_len = o;
	data->phase.value = phase;
}

void resample_free(struct resample *r)
{
	free(r->data);
	r->data = NULL;
}

static inline uint32_t calc_gcd(uint32_t a, uint32_t b)
{
	while (b != 0) {
		uint32_t temp = a;
		a = b;
		b = temp % b;
	}
	return a;
}

void resample_update_rate(struct resample *r, double rate)
{
	struct native_data *data = r->data;
	struct fixp in_rate;
	uint32_t out_rate;

	if (data->rate == rate)
		return;

	data->rate = rate;
	in_rate = UINT32_TO_FIXP(r->i_rate);
	out_rate = r->o_rate;

	in_rate.value = (uint64_t)round(in_rate.value / rate);

	data->in_rate = in_rate;

	data->inc = in_rate.value / UINT32_TO_FIXP(out_rate).value;
	data->frac.value = in_rate.value % UINT32_TO_FIXP(out_rate).value;
}

static uint64_t fixp_floor_a_plus_bc(struct fixp a, uint32_t b, struct fixp c)
{
	/* (a + b*c) >> FIXP_SHIFT, with bigger overflow threshold */
	uint64_t hi, lo;
	hi = (a.value >> FIXP_SHIFT) + b * (c.value >> FIXP_SHIFT);
	lo = (a.value & FIXP_MASK) + b * (c.value & FIXP_MASK);
	return hi + (lo >> FIXP_SHIFT);
}

uint32_t resample_in_len(struct resample *r, uint32_t out_len)
{
	struct native_data *data = r->data;
	uint32_t in_len;

	in_len = fixp_floor_a_plus_bc(data->phase, out_len, data->frac) / data->out_rate;
	in_len += out_len * data->inc +	(data->n_taps - data->hist);

	return in_len;
}

uint32_t resample_out_len(struct resample *r, uint32_t in_len)
{
	struct native_data *data = r->data;
	uint32_t out_len;

	in_len = in_len - MIN(in_len, data->n_taps - data->hist);
	out_len = in_len * data->out_rate - FIXP_TO_UINT32(data->phase);
	out_len = (UINT32_TO_FIXP(out_len).value + data->in_rate.value - 1) / data->in_rate.value;

	return out_len;
}

void resample_process(struct resample *r,
		const void *src[], uint32_t *in_len,
		void *dst[], uint32_t *out_len)
{
	struct native_data *data = r->data;
	uint32_t n_taps = data->n_taps;
	float **history = data->history;
	const float **s = (const float **)src;
	uint32_t c, refill, hist, in, out, remain;

	hist = data->hist;
	refill = 0;

	if (hist) {
		/* first work on the history if any. */
		if (hist <= n_taps) {
			/* we need at least n_taps to completely process the
			 * history before we can work on the new input. When
			 * we have less, refill the history. */
			refill = MIN(*in_len, n_taps-1);
			for (c = 0; c < r->channels; c++)
				memcpy(&history[c][hist], s[c], refill * sizeof(float));

			if (hist + refill < n_taps) {
				/* not enough in the history, keep the input in
				 * the history and produce no output */
				data->hist = hist + refill;
				*in_len = refill;
				*out_len = 0;
				return;
			}
		}
		/* now we have at least n_taps of data in the history
		 * and we try to process it */
		in = hist + refill;
		out = *out_len;
		do_resample_inter(r, (const void**)history, 0, &in, dst, 0, &out);
	} else {
		out = in = 0;
	}

	if (in >= hist) {
		int skip = in - hist;
		/* we are past the history and can now work on the new
		 * input data */
		in = *in_len;
		do_resample_inter(r, src, skip, &in, dst, out, out_len);

		remain = *in_len - in;
		if (remain > 0 && remain <= n_taps) {
			/* not enough input data remaining for more output,
			 * copy to history */
			for (c = 0; c < r->channels; c++)
				memcpy(history[c], &s[c][in], remain * sizeof(float));
		} else {
			/* we have enough input data remaining to produce
			 * more output ask to resubmit. */
			remain = 0;
			*in_len = in;
		}
	} else {
		/* we are still working on the history */
		*out_len = out;
		remain = hist - in;
		if (*in_len < n_taps) {
			/* not enough input data, add it to the history because
			 * resubmitting it is not going to make progress.
			 * We copied this into the history above. */
			remain += refill;
		} else {
			/* input has enough data to possibly produce more output
			 * from the history so ask to resubmit */
			*in_len = 0;
		}
		if (remain) {
			/* move history */
			for (c = 0; c < r->channels; c++)
				memmove(history[c], &history[c][in], remain * sizeof(float));
		}

	}
	data->hist = remain;
	return;
}

void resample_reset (struct resample *r)
{
	struct native_data *d = r->data;
	if (d == NULL)
		return;
	memset(d->hist_mem, 0, r->channels * sizeof(float) * d->n_taps * 2);
	if (r->options & RESAMPLE_OPTION_PREFILL)
		d->hist = d->n_taps - 1;
	else
		d->hist = d->n_taps / 2;
	d->phase.value = 0;
}

uint32_t resample_delay (struct resample *r)
{
	struct native_data *d = r->data;
	return d->n_taps / 2 - 1;
}

float resample_phase (struct resample *r)
{
	struct native_data *d = r->data;
	float pho = 0;

	pho = -FIXP_TO_FLOAT(d->phase) / d->out_rate;

	/* XXX: this is how it seems to behave, but not clear why */
	if (d->hist >= d->n_taps - 1)
		pho += 1.0f;

	return pho;
}

static inline void *ptroff_align(void *ptr, size_t offset, size_t align)
{
	return (void *)ROUND_UP_N((uintptr_t)ptr + offset, (uintptr_t)align);
}

int resample_init(struct resample *r)
{
	struct native_data *d;
	const struct quality *q;
	double scale, cutoff;
	uint32_t i, n_taps, n_phases, in_rate, out_rate, gcd, filter_stride;
	uint32_t history_stride, oversample;
	size_t filter_size, history_size, alloc_size;
	struct resample_config *c = &r->config;

	c->window = MIN(c->window, N_ELEMENTS(window_info)-1);
	r->quality = CLAMP(r->quality, 0, (int)(window_info[c->window].n_qualities - 1));

	q = &window_info[c->window].qualities[r->quality];
	window_info[c->window].config(r, q);

	cutoff = r->o_rate < r->i_rate ? q->cutoff_down : q->cutoff_up;
	c->cutoff = c->cutoff <= 0.0 ? cutoff: c->cutoff;
	n_taps = c->n_taps == 0 ? q->n_taps : c->n_taps;

	gcd = calc_gcd(r->i_rate, r->o_rate);

	in_rate = r->i_rate / gcd;
	out_rate = r->o_rate / gcd;

	scale = MIN(c->cutoff * out_rate / in_rate, c->cutoff);

	/* multiple of 8 taps */
	n_taps = ROUND_UP_N((uint32_t)ceil(n_taps / scale), 8);
	n_taps = MIN(n_taps, MAX_TAPS);

	/* When upsampling with default cutoff, rounding n_taps up may
	 * give extra taps. Use them for a higher cutoff:
	 *   cutoff = n_taps / (n_taps + K) */
	if (out_rate >= in_rate && c->cutoff == cutoff) {
		double k = 4.0;
		double max_cutoff = MIN((double)n_taps / (n_taps + k), 0.99);
		if (max_cutoff > scale)
			scale = max_cutoff;
	}

	/* try to get at least 256 phases so that interpolation is
	 * accurate enough */
	n_phases = MIN(MAX_PHASES, out_rate);
	oversample = (255 + n_phases) / n_phases;
	n_phases *= oversample;

	filter_stride = ROUND_UP_N(n_taps * sizeof(float), 64);
	if (overflow_mul((size_t)filter_stride, (size_t)(n_phases + 1), &filter_size))
		return -ENOMEM;

	history_stride = ROUND_UP_N(2 * n_taps * sizeof(float), 64);
	if (overflow_mul((size_t)r->channels, (size_t)history_stride, &history_size))
		return -ENOMEM;

	alloc_size = sizeof(struct native_data);
	if (overflow_add(alloc_size, filter_size, &alloc_size) ||
	    overflow_add(alloc_size, history_size, &alloc_size) ||
	    overflow_add(alloc_size, (size_t)r->channels * sizeof(float*), &alloc_size) ||
	    overflow_add(alloc_size, (size_t)64, &alloc_size))
		return -ENOMEM;

	d = calloc(1, alloc_size);
	if (d == NULL)
		return -errno;

	r->data = d;
	c->n_taps = d->n_taps = n_taps;
	d->n_phases = n_phases;
	d->in_rate = UINT32_TO_FIXP(r->i_rate);
	d->out_rate = r->o_rate;
	d->pm = (float)n_phases / r->o_rate / FIXP_SCALE;
	d->filter = ptroff_align(d, sizeof(struct native_data), 64);
	d->hist_mem = ptroff_align(d->filter, filter_size, 64);
	d->history = (float **)((char *)d->hist_mem + history_size);
	d->filter_stride = filter_stride / sizeof(float);
	for (i = 0; i < r->channels; i++)
		d->history[i] = (float *)((char *)d->hist_mem + i * history_stride);

	build_filter(r, d->filter, d->filter_stride, n_taps, n_phases, scale,
			(double *)d->hist_mem);

	resample_reset(r);
	resample_update_rate(r, 1.0);

	return 0;
}
