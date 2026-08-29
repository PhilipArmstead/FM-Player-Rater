// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"


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
