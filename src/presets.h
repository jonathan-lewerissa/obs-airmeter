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

#define AIRMETER_TOLERANCE_LU 1.0
#define AIRMETER_AMBER_LU 3.0
#define AIRMETER_DEFAULT_CEILING_DBTP -1.0

struct airmeter_preset {
	const char *id;
	const char *text_key;
	double target; /* The integrated loudness in LUFS. It is NAN for Custom. */
};

extern const struct airmeter_preset airmeter_presets[];
extern const size_t airmeter_preset_count;

/* If no preset has this id, the function returns the default preset. */
const struct airmeter_preset *airmeter_find_preset(const char *id);

enum airmeter_status {
	AIRMETER_NO_SIGNAL,
	AIRMETER_ON_TARGET,
	AIRMETER_BIT_LOUD,
	AIRMETER_TOO_LOUD,
	AIRMETER_BIT_QUIET,
	AIRMETER_TOO_QUIET,
};

enum airmeter_status airmeter_status(double value, double target);

#ifdef __cplusplus
}
#endif
