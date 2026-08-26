// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

typedef struct {
	void *handle; // Platform-specific process handle
	uintptr_t moduleBaseAddress;
	uint32_t pid;
} ProcessContext;
