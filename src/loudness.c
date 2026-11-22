/*
obs-airmeter
Copyright (C) 2026 Jonathan Lewerissa <jonathan.rehuel16@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "loudness.h"

#include <ebur128.h>
#include <math.h>
#include <stdlib.h>

/* HISTOGRAM mode prevents malloc in ebur128_add_frames_float(). It also makes the I and LRA reads fast. */
#define LOUDNESS_MODES                                                                                   \
	(EBUR128_MODE_M | EBUR128_MODE_S | EBUR128_MODE_I | EBUR128_MODE_LRA | EBUR128_MODE_TRUE_PEAK | \
	 EBUR128_MODE_HISTOGRAM)

struct loudness {
	ebur128_state *st;
	double tick_peak; /* The value is linear, not in dB. */
};

struct loudness *loudness_create(unsigned int channels, unsigned long rate, const int *channel_map)
{
	struct loudness *l = calloc(1, sizeof(*l));
	if (!l)
		return NULL;
	l->st = ebur128_init(channels, rate, LOUDNESS_MODES);
	if (!l->st) {
		free(l);
		return NULL;
	}
	for (unsigned int c = 0; channel_map && c < channels; c++)
		ebur128_set_channel(l->st, c, channel_map[c]);
	return l;
}

void loudness_destroy(struct loudness *l)
{
	if (!l)
		return;
	ebur128_destroy(&l->st);
	free(l);
}

void loudness_add(struct loudness *l, const float *frames, size_t count)
{
	if (ebur128_add_frames_float(l->st, frames, count) != EBUR128_SUCCESS)
		return;
	for (unsigned int c = 0; c < l->st->channels; c++) {
		double p;
		if (ebur128_prev_true_peak(l->st, c, &p) == EBUR128_SUCCESS && p > l->tick_peak)
			l->tick_peak = p;
	}
}

static double finite_or_nan(int err, double v)
{
	return err == EBUR128_SUCCESS && isfinite(v) ? v : NAN;
}

static double peak_db(double linear)
{
	return linear > 0.0 ? 20.0 * log10(linear) : NAN;
}

void loudness_read(struct loudness *l, struct loudness_values *out)
{
	double v;
	int err;

	err = ebur128_loudness_momentary(l->st, &v);
	out->m = finite_or_nan(err, v);
	err = ebur128_loudness_shortterm(l->st, &v);
	out->s = finite_or_nan(err, v);
	err = ebur128_loudness_global(l->st, &v);
	out->i = finite_or_nan(err, v);
	err = ebur128_loudness_range(l->st, &v);
	/* Before the first gated block, libebur128 gives LRA 0. Show no LRA until I is available. */
	out->lra = isnan(out->i) ? NAN : finite_or_nan(err, v);

	double max = 0.0;
	for (unsigned int c = 0; c < l->st->channels; c++) {
		if (ebur128_true_peak(l->st, c, &v) == EBUR128_SUCCESS && v > max)
			max = v;
	}
	out->tp_max = peak_db(max);
	out->tp_tick = peak_db(l->tick_peak);
	l->tick_peak = 0.0;
}
