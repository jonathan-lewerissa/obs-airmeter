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

#include <array>
#include <cstddef>
#include <vector>

#include "loudness.h"

template<size_t N> class Ring {
public:
	void push(double v)
	{
		data[(head + count) % N] = v;
		if (count < N)
			count++;
		else
			head = (head + 1) % N;
	}

	void clear() { head = count = 0; }

	size_t size() const { return count; }

	/* Index 0 is the oldest value. */
	double at(size_t i) const { return data[(head + i) % N]; }

	std::vector<double> values() const
	{
		std::vector<double> v(count);
		for (size_t i = 0; i < count; i++)
			v[i] = at(i);
		return v;
	}

	static constexpr size_t capacity = N;

private:
	std::array<double, N> data{};
	size_t head = 0;
	size_t count = 0;
};

struct History {
	static constexpr size_t ticks_per_second = 10;
	static constexpr size_t window = 5 * 60 * ticks_per_second;

	Ring<window> s, m, tp;
	std::vector<double> i;

	void add(const loudness_values &v)
	{
		s.push(v.s);
		m.push(v.m);
		tp.push(v.tp_tick);
		if (tick++ % ticks_per_second == 0)
			i.push_back(v.i);
	}

	void clear()
	{
		s.clear();
		m.clear();
		tp.clear();
		i.clear();
		tick = 0;
	}

private:
	size_t tick = 0;
};
