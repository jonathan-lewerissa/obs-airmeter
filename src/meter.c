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

#include "meter.h"

#include <ebur128.h>
#include <math.h>
#include <obs.h>
#include <plugin-support.h>
#include <util/threading.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct loudness *current; /* Use only while you hold lock. */
static struct audio_convert_info conv;
static size_t active_mix;
static bool connected;

/* Gives the BS.1770 position for each channel in the OBS channel order (media-io/audio-io.h). */
static const int *channel_map(enum speaker_layout speakers)
{
	static const int map_2_1[] = {EBUR128_LEFT, EBUR128_RIGHT, EBUR128_UNUSED};
	static const int map_4_0[] = {EBUR128_LEFT, EBUR128_RIGHT, EBUR128_CENTER, EBUR128_Mp180};
	static const int map_4_1[] = {EBUR128_LEFT, EBUR128_RIGHT, EBUR128_CENTER, EBUR128_UNUSED, EBUR128_Mp180};
	static const int map_7_1[] = {EBUR128_LEFT,  EBUR128_RIGHT, EBUR128_CENTER, EBUR128_UNUSED,
				      EBUR128_Mp135, EBUR128_Mm135, EBUR128_Mp090,  EBUR128_Mm090};

	switch (speakers) {
	case SPEAKERS_2POINT1:
		return map_2_1;
	case SPEAKERS_4POINT0:
		return map_4_0;
	case SPEAKERS_4POINT1:
		return map_4_1;
	case SPEAKERS_7POINT1:
		return map_7_1;
	default:
		return NULL; /* For mono, stereo and 5.1, the libebur128 default map is correct. */
	}
}

static void audio_cb(void *param, size_t mix, struct audio_data *data)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(mix);

	pthread_mutex_lock(&lock);
	if (current)
		loudness_add(current, (const float *)data->data[0], data->frames);
	pthread_mutex_unlock(&lock);
}

void meter_reset(void)
{
	struct loudness *next =
		loudness_create(get_audio_channels(conv.speakers), conv.samples_per_sec, channel_map(conv.speakers));
	if (!next) {
		obs_log(LOG_ERROR, "cannot create the loudness state");
		return;
	}

	pthread_mutex_lock(&lock);
	struct loudness *old = current;
	current = next;
	pthread_mutex_unlock(&lock);

	loudness_destroy(old);
}

bool meter_start(size_t mix)
{
	struct obs_audio_info oai;
	if (!obs_get_audio_info(&oai)) {
		obs_log(LOG_ERROR, "OBS audio is not active");
		return false;
	}

	conv.samples_per_sec = oai.samples_per_sec;
	conv.format = AUDIO_FORMAT_FLOAT;
	conv.speakers = oai.speakers;
	conv.allow_clipping = true;

	meter_reset();
	if (!current)
		return false;

	active_mix = mix;
	obs_add_raw_audio_callback(active_mix, &conv, audio_cb, NULL);
	connected = true;
	obs_log(LOG_INFO, "measuring track %zu, %u Hz, %u channels", mix + 1, conv.samples_per_sec,
		get_audio_channels(conv.speakers));
	return true;
}

void meter_stop(void)
{
	if (connected)
		obs_remove_raw_audio_callback(active_mix, audio_cb, NULL);
	connected = false;

	pthread_mutex_lock(&lock);
	struct loudness *old = current;
	current = NULL;
	pthread_mutex_unlock(&lock);

	loudness_destroy(old);
}

void meter_set_mix(size_t mix)
{
	meter_stop();
	meter_start(mix);
}

void meter_read(struct loudness_values *out)
{
	pthread_mutex_lock(&lock);
	if (current)
		loudness_read(current, out);
	else
		out->m = out->s = out->i = out->lra = out->tp_tick = out->tp_max = NAN;
	pthread_mutex_unlock(&lock);
}
