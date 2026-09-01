// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>


typedef struct {
	void *data;
	uint64_t refCount;
} SharedPointer;
