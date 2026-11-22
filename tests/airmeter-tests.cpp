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

/* The tolerances come from EBU Tech 3341 V4 (2023) and EBU Tech 3342 V4 (2023). Do not change them. */

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

#include <ebur128.h>

#include "history.hpp"
#include "loudness.h"
#include "presets.h"

static int failures = 0;

static void check(bool ok, const char *name, double got, double expected)
{
	std::printf("%s %-40s got %8.3f expected %8.3f\n", ok ? "PASS" : "FAIL", name, got, expected);
	if (!ok)
		failures++;
}

static void near(const char *name, double got, double expected, double minus, double plus)
{
	check(got >= expected - minus && got <= expected + plus, name, got, expected);
}

static constexpr unsigned long rate = 48000;
static constexpr size_t block = 480;

struct Tone {
	double seconds;
	double dbfs; /* Peak level of each channel. NAN is silence. */
};

/* Sends 1 kHz stereo tones in 10 ms blocks. After each block, calls on_block with the current values. */
static void play(struct loudness *l, const std::vector<Tone> &tones,
		 const std::function<void(size_t tone, const loudness_values &)> &on_block = nullptr,
		 unsigned int channels = 2, const std::vector<double> &channel_db = {})
{
	std::vector<float> buf(block * channels);
	size_t n = 0;
	for (size_t t = 0; t < tones.size(); t++) {
		size_t frames = (size_t)std::llround(tones[t].seconds * rate);
		for (size_t done = 0; done < frames; done += block) {
			size_t count = std::min(block, frames - done);
			for (size_t f = 0; f < count; f++, n++) {
				double phase = std::sin(2.0 * M_PI * 1000.0 * double(n) / rate);
				for (unsigned int c = 0; c < channels; c++) {
					double db = channel_db.empty() ? tones[t].dbfs : channel_db[c];
					buf[f * channels + c] =
						std::isnan(db) ? 0.0f : float(std::pow(10.0, db / 20.0) * phase);
				}
			}
			loudness_add(l, buf.data(), count);
			if (on_block) {
				loudness_values v;
				loudness_read(l, &v);
				on_block(t, v);
			}
		}
	}
}

static loudness_values measure(const std::vector<Tone> &tones)
{
	struct loudness *l = loudness_create(2, rate, nullptr);
	play(l, tones);
	loudness_values v;
	loudness_read(l, &v);
	loudness_destroy(l);
	return v;
}

static void tech3341_cases_1_to_5()
{
	loudness_values v = measure({{20, -23}});
	near("3341 #1 M", v.m, -23, 0.1, 0.1);
	near("3341 #1 S", v.s, -23, 0.1, 0.1);
	near("3341 #1 I", v.i, -23, 0.1, 0.1);

	v = measure({{20, -33}});
	near("3341 #2 M", v.m, -33, 0.1, 0.1);
	near("3341 #2 S", v.s, -33, 0.1, 0.1);
	near("3341 #2 I", v.i, -33, 0.1, 0.1);

	near("3341 #3 I", measure({{10, -36}, {60, -23}, {10, -36}}).i, -23, 0.1, 0.1);
	near("3341 #4 I", measure({{10, -72}, {10, -36}, {60, -23}, {10, -36}, {10, -72}}).i, -23, 0.1, 0.1);
	near("3341 #5 I", measure({{20, -26}, {20.1, -20}, {20, -26}}).i, -23, 0.1, 0.1);
}

static void tech3341_case_6()
{
	const int map[] = {EBUR128_LEFT, EBUR128_RIGHT, EBUR128_CENTER, EBUR128_LEFT_SURROUND, EBUR128_RIGHT_SURROUND};
	struct loudness *l = loudness_create(5, rate, map);
	play(l, {{20, 0}}, nullptr, 5, {-28, -28, -24, -30, -30});
	loudness_values v;
	loudness_read(l, &v);
	loudness_destroy(l);
	near("3341 #6 I (5.0)", v.i, -23, 0.1, 0.1);
}

/* Cases 9 and 12: the value must stay in tolerance after the first window. */
static void tech3341_constant(const char *name, double high_s, double low_s, int repeats, double settle,
			      double loudness_values::*field)
{
	std::vector<Tone> tones;
	for (int k = 0; k < repeats; k++) {
		tones.push_back({high_s, -20});
		tones.push_back({low_s, -30});
	}
	double lo = INFINITY, hi = -INFINITY;
	size_t blocks = 0;
	struct loudness *l = loudness_create(2, rate, nullptr);
	play(l, tones, [&](size_t, const loudness_values &v) {
		if (double(++blocks * block) / rate < settle)
			return;
		lo = std::min(lo, v.*field);
		hi = std::max(hi, v.*field);
	});
	loudness_destroy(l);
	near(name, lo, -23, 0.1, 0.1);
	near(name, hi, -23, 0.1, 0.1);
}

/* Cases 11 and 14: one segment for each i. The maximum in segment i must be -38 + i. */
static void tech3341_live(const char *name, double step, double tone, double segment, double loudness_values::*field)
{
	std::vector<Tone> tones;
	for (int i = 0; i < 20; i++) {
		tones.push_back({i * step, NAN});
		tones.push_back({tone, -38.0 + i});
		tones.push_back({segment - tone - i * step, NAN});
	}
	std::vector<double> max(20, -INFINITY);
	struct loudness *l = loudness_create(2, rate, nullptr);
	play(l, tones, [&](size_t t, const loudness_values &v) {
		if (!std::isnan(v.*field))
			max[t / 3] = std::max(max[t / 3], v.*field);
	});
	loudness_destroy(l);
	for (int i = 0; i < 20; i++) {
		char label[64];
		std::snprintf(label, sizeof(label), "%s segment %d", name, i);
		near(label, max[i], -38.0 + i, 0.1, 0.1);
	}
}

/* Cases 15 to 19. Each tone is 1 s long with a 10 ms fade-in and fade-out. */
static void tech3341_true_peak(const char *name, double divisor, double amplitude, double phase_deg, double expected)
{
	size_t frames = rate;
	size_t fade = rate / 100;
	std::vector<float> buf(frames * 2);
	for (size_t n = 0; n < frames; n++) {
		double gain = std::min({1.0, double(n) / fade, double(frames - 1 - n) / fade});
		double x = amplitude * gain * std::sin(2.0 * M_PI * double(n) / divisor + phase_deg * M_PI / 180.0);
		buf[2 * n] = buf[2 * n + 1] = float(x);
	}
	struct loudness *l = loudness_create(2, rate, nullptr);
	for (size_t done = 0; done < frames; done += block)
		loudness_add(l, buf.data() + 2 * done, std::min(block, frames - done));
	loudness_values v;
	loudness_read(l, &v);
	loudness_destroy(l);
	near(name, v.tp_max, expected, 0.4, 0.2);
}

static void tech3342()
{
	near("3342 #1 LRA", measure({{20, -20}, {20, -30}}).lra, 10, 1, 1);
	near("3342 #2 LRA", measure({{20, -20}, {20, -15}}).lra, 5, 1, 1);
	near("3342 #3 LRA", measure({{20, -40}, {20, -20}}).lra, 20, 1, 1);
	near("3342 #4 LRA", measure({{20, -50}, {20, -35}, {20, -20}, {20, -35}, {20, -50}}).lra, 15, 1, 1);
}

static void silence_gives_no_values()
{
	loudness_values v = measure({{5, NAN}});
	bool all_nan = std::isnan(v.m) && std::isnan(v.s) && std::isnan(v.i) && std::isnan(v.lra) &&
		       std::isnan(v.tp_tick) && std::isnan(v.tp_max);
	check(all_nan, "silence gives NAN for all values", 0, 0);
}

static void status_rules()
{
	struct {
		double value;
		enum airmeter_status expected;
	} cases[] = {
		{NAN, AIRMETER_NO_SIGNAL},   {-14.0, AIRMETER_ON_TARGET}, {-13.0, AIRMETER_ON_TARGET},
		{-15.0, AIRMETER_ON_TARGET}, {-12.9, AIRMETER_BIT_LOUD},  {-11.0, AIRMETER_BIT_LOUD},
		{-10.9, AIRMETER_TOO_LOUD},  {-15.1, AIRMETER_BIT_QUIET}, {-17.0, AIRMETER_BIT_QUIET},
		{-17.1, AIRMETER_TOO_QUIET},
	};
	for (auto &c : cases) {
		enum airmeter_status got = airmeter_status(c.value, -14.0);
		check(got == c.expected, "status against -14 LUFS", c.value, c.value);
	}
}

static void presets()
{
	check(airmeter_find_preset("ebu_r128")->target == -23.0, "preset ebu_r128", 0, 0);
	check(airmeter_find_preset("unknown") == &airmeter_presets[0], "unknown preset gives default", 0, 0);
	check(airmeter_find_preset(nullptr) == &airmeter_presets[0], "NULL preset gives default", 0, 0);
	check(airmeter_presets[0].target == -14.0, "default preset is -14 LUFS", 0, 0);
	check(std::isnan(airmeter_find_preset("custom")->target), "custom preset has no target", 0, 0);
}

static void history()
{
	History h;
	for (int k = 0; k <= int(History::window); k++)
		h.add({double(k), double(k), double(k), 0, double(k), 0});
	check(h.s.size() == History::window, "ring keeps 5 minutes", double(h.s.size()), double(History::window));
	check(h.s.at(0) == 1.0, "ring drops the oldest value", h.s.at(0), 1.0);
	check(h.s.at(History::window - 1) == double(History::window), "ring keeps the newest value",
	      h.s.at(History::window - 1), double(History::window));
	check(h.i.size() == History::window / 10 + 1, "I has 1 point for each second", double(h.i.size()),
	      double(History::window / 10 + 1));
	h.clear();
	check(h.s.size() == 0 && h.i.empty(), "clear removes all history", 0, 0);
}

int main()
{
	tech3341_cases_1_to_5();
	tech3341_case_6();
	tech3341_constant("3341 #9 S", 1.34, 1.66, 5, 3.0, &loudness_values::s);
	tech3341_live("3341 #11 max S", 0.15, 3.0, 6.0, &loudness_values::s);
	tech3341_constant("3341 #12 M", 0.18, 0.22, 25, 1.0, &loudness_values::m);
	tech3341_live("3341 #14 max M", 0.02, 0.4, 0.8, &loudness_values::m);
	tech3341_true_peak("3341 #15 TP", 4, 0.50, 0.0, -6.0);
	tech3341_true_peak("3341 #16 TP", 4, 0.50, 45.0, -6.0);
	tech3341_true_peak("3341 #17 TP", 6, 0.50, 60.0, -6.0);
	tech3341_true_peak("3341 #18 TP", 8, 0.50, 67.5, -6.0);
	tech3341_true_peak("3341 #19 TP", 4, 1.41, 45.0, 3.0);
	tech3342();
	silence_gives_no_values();
	status_rules();
	presets();
	history();

	std::printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
