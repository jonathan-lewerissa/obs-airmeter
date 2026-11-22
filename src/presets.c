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

#include "presets.h"

#include <math.h>
#include <string.h>

/* This table must agree with the presets table in README.md. */
const struct airmeter_preset airmeter_presets[] = {
	/* Not published by YouTube. Ian Shepherd (MeterPlugs) measured this value:
	 * https://www.meterplugs.com/blog/2019/09/18/youtube-changes-loudness-reference-to-14-lufs.html */
	{"youtube", "Preset.YouTube", -14.0},
	/* https://support.spotify.com/artists/article/loudness-normalization/ */
	{"spotify", "Preset.Spotify", -14.0},
	/* Not published by Apple. Ian Shepherd (MeterPlugs) measured this value:
	 * https://www.meterplugs.com/blog/2022/03/23/apple-switch-to-lufs.html */
	{"apple_music", "Preset.AppleMusic", -16.0},
	/* AES TD1008: https://www.aes.org/technical/documentDownloads.cfm?docID=731 */
	{"aes_speech", "Preset.AesSpeech", -18.0},
	/* AES TD1008: https://www.aes.org/technical/documentDownloads.cfm?docID=731 */
	{"aes_music", "Preset.AesMusic", -16.0},
	/* EBU R 128: https://tech.ebu.ch/publications/r128 */
	{"ebu_r128", "Preset.EbuR128", -23.0},
	{"custom", "Preset.Custom", NAN},
};

const size_t airmeter_preset_count = sizeof(airmeter_presets) / sizeof(airmeter_presets[0]);

const struct airmeter_preset *airmeter_find_preset(const char *id)
{
	for (size_t i = 0; id && i < airmeter_preset_count; i++) {
		if (strcmp(airmeter_presets[i].id, id) == 0)
			return &airmeter_presets[i];
	}
	return &airmeter_presets[0];
}

enum airmeter_status airmeter_status(double value, double target)
{
	if (isnan(value))
		return AIRMETER_NO_SIGNAL;
	double d = value - target;
	if (fabs(d) <= AIRMETER_TOLERANCE_LU)
		return AIRMETER_ON_TARGET;
	if (d > AIRMETER_AMBER_LU)
		return AIRMETER_TOO_LOUD;
	if (d > 0)
		return AIRMETER_BIT_LOUD;
	if (d < -AIRMETER_AMBER_LU)
		return AIRMETER_TOO_QUIET;
	return AIRMETER_BIT_QUIET;
}
