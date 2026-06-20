// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


// Logger
void platform_consoleWrite(const char *message, LogLevel colour);
void platform_consoleWriteError(const char *message, LogLevel colour);

// Memory
void readFromMemory(void *handle, uintptr_t address, size_t length, uint8_t *bytes);
uint8_t readByte(void *handle, uintptr_t address);
void writeToMemory(void *handle, uintptr_t address, size_t length, const uint8_t *bytes);

// Process
void platform_openProcess(ProcessContext *context);
