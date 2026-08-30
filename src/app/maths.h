// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

#include <math.h>


// Assumes little-endian byte order
static inline uint64_t hexBytesToInt(const uint8_t *bytes, const uint8_t length) {
	uint64_t value = 0;
	for (uint8_t i = 0; i < length; ++i) {
		value |= (uint64_t)bytes[i] << (8 * i);
	}
	return value;
}

// Converts a number 0-100 to 1-20
static inline uint8_t convertTo20Scale(const uint8_t value) {
	return (uint8_t)((value + 4) / 5);
}


static void hueToHex(double hue, char hex[8]) {
	hue = fmod(hue, 360.0);
	if (hue < 0.0) {
		hue += 360.0;
	}

	const double x = 1.0 - fabs(fmod(hue / 60.0, 2.0) - 1.0);
	double r, g, b;

	if (hue < 60.0) {
		r = 1.0;
		g = x;
		b = 0.0;
	} else if (hue < 120.0) {
		r = x;
		g = 1.0;
		b = 0.0;
	} else if (hue < 180.0) {
		r = 0.0;
		g = 1.0;
		b = x;
	} else if (hue < 240.0) {
		r = 0.0;
		g = x;
		b = 1.0;
	} else if (hue < 300.0) {
		r = x;
		g = 0.0;
		b = 1.0;
	} else {
		r = 1.0;
		g = 0.0;
		b = x;
	}

	snprintf(
		hex,
		8,
		"#%02x%02x%02x",
		(uint8_t)round(r * 255.0),
		(uint8_t)round(g * 255.0),
		(uint8_t)round(b * 255.0)
	);
}
