// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>


#define bool _Bool
#define false 0
#define true 1

// Logger
typedef enum LogLevel {
	LogLevelFatal,
	LogLevelError,
	LogLevelWarn,
	LogLevelInfo,
	LogLevelCount
} LogLevel;

// Process
typedef struct {
	void *handle; // Platform-specific process handle
	uintptr_t moduleBaseAddress;
	uint32_t pid;
} ProcessContext;
