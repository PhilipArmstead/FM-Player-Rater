// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"


// Logger
void platform_consoleWrite(const char *message, LogLevel colour);
void platform_consoleWriteError(const char *message, LogLevel colour);

// Process
void platform_openProcess(ProcessContext *context);
