// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

#define THREAD_COUNT 4


void clearCaches(void);
void cacheClubs(void);
void cacheNations(void);
void cachePlayers(uint8_t half);
void runMultiThreadedCache(void);
#ifdef ARCH_WIN
#include <windows.h>
DWORD WINAPI threadFunction(LPVOID arg);
#else
void *threadFunction(void *arg);
#endif
