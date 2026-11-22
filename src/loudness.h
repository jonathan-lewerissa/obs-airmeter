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

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This code does not lock. The caller must hold a lock for each call. */
struct loudness;

/* A value is NAN when it is not available. */
struct loudness_values {
	double m;
	double s;
	double i;
	double lra;
	double tp_tick; /* The highest true peak in dBTP since the last loudness_read(). */
	double tp_max;  /* The highest true peak in dBTP since loudness_create(). */
};

/* channel_map has one libebur128 `enum channel` value for each channel. If channel_map is NULL, libebur128 uses its default map. */
struct loudness *loudness_create(unsigned int channels, unsigned long rate, const int *channel_map);
void loudness_destroy(struct loudness *l);

/* Call on the audio thread. This function does not allocate memory. The frames are interleaved float. */
void loudness_add(struct loudness *l, const float *frames, size_t count);

void loudness_read(struct loudness *l, struct loudness_values *out);

#ifdef __cplusplus
}
#endif
